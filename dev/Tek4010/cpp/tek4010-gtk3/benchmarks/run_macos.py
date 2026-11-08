#!/usr/bin/env python3
"""Benchmark macOS resource use for emulator demos and PLT workloads."""

import argparse
import csv
from datetime import datetime, timezone
import os
from pathlib import Path
import platform
import signal
import sys
import time


REPO_ROOT = Path(__file__).resolve().parents[5]
DEMO_NAMES = ("demo", "fdemo", "gdemo", "idemo")
GPU_STATUS = "unavailable"
GPU_NOTE = "No validated per-process GPU percentage; system-wide readings excluded"
CSV_FIELDS = (
    "timestamp_utc",
    "macos_version",
    "architecture",
    "implementation",
    "demo",
    "trial",
    "run_status",
    "exit_code",
    "wall_seconds",
    "cpu_user_seconds",
    "cpu_system_seconds",
    "cpu_average_percent",
    "peak_rss_bytes",
    "gpu_utilization_percent",
    "gpu_status",
    "gpu_note",
)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path("build/macos-gcc-16"),
        help="CMake build directory, relative to the repository root",
    )
    parser.add_argument(
        "--demo",
        action="append",
        choices=DEMO_NAMES,
        help="demo workload to run; repeat to select multiple (default: demo)",
    )
    parser.add_argument(
        "--plt-files",
        action="store_true",
        help=(
            "benchmark test.plt, animation.plt, and top-level pltfiles/*.plt "
            "with the GTK3/C++ executable in performance mode"
        ),
    )
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=120,
        help="maximum run time for finite demos",
    )
    parser.add_argument(
        "--idemo-seconds",
        type=float,
        default=15,
        help="fixed measurement window when idemo is selected",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="write CSV here instead of stdout; relative paths use the repository root",
    )
    args = parser.parse_args()

    if args.repeats < 1:
        parser.error("--repeats must be at least 1")
    if args.timeout_seconds <= 0 or args.idemo_seconds <= 0:
        parser.error("time limits must be greater than zero")
    if args.plt_files and args.demo:
        parser.error("--plt-files cannot be combined with --demo")
    return args


def run_process(binary, command, time_limit):
    """Run one emulator command and return direct-process resource usage."""
    pid = os.fork()
    if pid == 0:
        try:
            os.setsid()
            os.chdir(REPO_ROOT)
            os.dup2(2, 1)
            os.execv(str(binary), [str(binary), *command])
        except OSError as error:
            os.write(2, f"Could not launch {binary}: {error}\n".encode())
            os._exit(127)

    started = time.monotonic()
    deadline = started + time_limit
    was_limited = False

    try:
        while True:
            waited_pid, status, usage = wait_for_child(pid, os.WNOHANG)
            if waited_pid == pid:
                break
            if time.monotonic() >= deadline:
                was_limited = True
                status, usage = terminate_and_wait(pid)
                break
            time.sleep(0.05)
    except KeyboardInterrupt:
        terminate_and_wait(pid)
        terminate_group(pid, signal.SIGKILL)
        raise

    wall_seconds = time.monotonic() - started
    if os.WIFEXITED(status):
        exit_code = os.WEXITSTATUS(status)
    else:
        exit_code = -os.WTERMSIG(status)
    return was_limited, exit_code, wall_seconds, usage


def plt_workloads():
    workloads = [Path("test.plt"), Path("animation.plt")]
    workloads.extend(
        path.relative_to(REPO_ROOT)
        for path in sorted((REPO_ROOT / "pltfiles").glob("*.plt"))
    )
    return workloads


def wait_for_child(pid, options):
    while True:
        try:
            return os.wait4(pid, options)
        except InterruptedError:
            continue


def terminate_and_wait(pid):
    terminate_group(pid, signal.SIGTERM)
    deadline = time.monotonic() + 2
    while True:
        waited_pid, status, usage = wait_for_child(pid, os.WNOHANG)
        if waited_pid == pid:
            terminate_group(pid, signal.SIGKILL)
            return status, usage
        if time.monotonic() >= deadline:
            terminate_group(pid, signal.SIGKILL)
            waited_pid, status, usage = wait_for_child(pid, 0)
            if waited_pid != pid:
                raise RuntimeError(f"Unexpected child process result: {waited_pid}")
            return status, usage
        time.sleep(0.05)


def terminate_group(process_group, signum):
    try:
        os.killpg(process_group, signum)
    except ProcessLookupError:
        pass


def row_for_result(implementation, demo, trial, result):
    was_limited, exit_code, wall_seconds, usage = result
    if was_limited:
        run_status = "duration_limit" if demo == "idemo" else "timed_out"
    elif exit_code == 0:
        run_status = "completed"
    elif exit_code < 0:
        run_status = "signal_terminated"
    else:
        run_status = "exit_error"

    cpu_user_seconds = usage.ru_utime
    cpu_system_seconds = usage.ru_stime
    cpu_seconds = cpu_user_seconds + cpu_system_seconds
    return {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "macos_version": platform.mac_ver()[0],
        "architecture": platform.machine(),
        "implementation": implementation,
        "demo": demo,
        "trial": trial,
        "run_status": run_status,
        "exit_code": exit_code,
        "wall_seconds": f"{wall_seconds:.3f}",
        "cpu_user_seconds": f"{cpu_user_seconds:.3f}",
        "cpu_system_seconds": f"{cpu_system_seconds:.3f}",
        "cpu_average_percent": f"{cpu_seconds / wall_seconds * 100:.2f}"
        if wall_seconds
        else "0.00",
        "peak_rss_bytes": usage.ru_maxrss,
        "gpu_utilization_percent": "N/A",
        "gpu_status": GPU_STATUS,
        "gpu_note": GPU_NOTE,
    }


def write_rows(rows, output):
    if output is None:
        csv_file = sys.stdout
        writer = csv.DictWriter(csv_file, fieldnames=CSV_FIELDS)
        writer.writeheader()
        writer.writerows(rows)
        return

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=CSV_FIELDS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Wrote benchmark results to {output}", file=sys.stderr)


def main():
    args = parse_args()
    if sys.platform != "darwin":
        print("This benchmark requires macOS.", file=sys.stderr)
        return 2
    if not hasattr(os, "wait4") or not hasattr(os, "fork"):
        print("This Python build does not provide fork/wait4 process metrics.", file=sys.stderr)
        return 2

    build_dir = args.build_dir
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir
    cpp_binary = (
        build_dir / "dev/Tek4010/cpp/tek4010-gtk3/tek4010-gtk3-cpp"
    )
    binaries = (
        (("cpp-performance", cpp_binary),)
        if args.plt_files
        else (("legacy-c", build_dir / "tek4010"), ("cpp", cpp_binary))
    )
    for _, binary in binaries:
        if not binary.is_file() or not os.access(binary, os.X_OK):
            print(f"Executable not found or not executable: {binary}", file=sys.stderr)
            return 2

    rows = []
    failed = False
    if args.plt_files:
        for workload in plt_workloads():
            workload_path = REPO_ROOT / workload
            if not workload_path.is_file():
                print(f"PLT workload not found: {workload_path}", file=sys.stderr)
                return 2
            for trial in range(1, args.repeats + 1):
                print(
                    f"Running cpp-performance {workload}, trial {trial} "
                    f"(limit {args.timeout_seconds:g}s)",
                    file=sys.stderr,
                )
                row = row_for_result(
                    "cpp-performance",
                    workload.as_posix(),
                    trial,
                    run_process(
                        cpp_binary,
                        [
                            "-performance",
                            "-exit",
                            "cat",
                            workload.as_posix(),
                        ],
                        args.timeout_seconds,
                    ),
                )
                rows.append(row)
                failed |= row["run_status"] in {
                    "timed_out",
                    "signal_terminated",
                    "exit_error",
                }
    else:
        demos = args.demo or ["demo"]
        for demo in demos:
            demo_path = REPO_ROOT / "demos" / f"{demo}.sh"
            if not demo_path.is_file():
                print(f"Demo script not found: {demo_path}", file=sys.stderr)
                return 2
            for trial in range(1, args.repeats + 1):
                ordered_binaries = binaries if trial % 2 else tuple(reversed(binaries))
                for implementation, binary in ordered_binaries:
                    time_limit = (
                        args.idemo_seconds
                        if demo == "idemo"
                        else args.timeout_seconds
                    )
                    print(
                        f"Running {implementation} {demo}, trial {trial} "
                        f"(limit {time_limit:g}s)",
                        file=sys.stderr,
                    )
                    row = row_for_result(
                        implementation,
                        demo,
                        trial,
                        run_process(
                            binary,
                            ["-exit", f"demos/{demo}.sh"],
                            time_limit,
                        ),
                    )
                    rows.append(row)
                    failed |= row["run_status"] in {
                        "timed_out",
                        "signal_terminated",
                        "exit_error",
                    }

    output = args.output
    if output is not None and not output.is_absolute():
        output = REPO_ROOT / output
    write_rows(rows, output)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
