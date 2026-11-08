#set document(title: "Tek4010 Benchmarks")
#set page(margin: 28mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 Benchmarks]
]

= Scope

The benchmark compares the legacy C `tek4010` executable with
`tek4010-gtk3-cpp` while each runs the same existing demo script. It is
macOS-only and uses Python's `fork` and `wait4` to collect metrics for the
emulator process itself.

The recorded measurements are wall-clock duration, user and system CPU time,
average CPU utilization during the run, and peak resident memory. CPU
utilization is calculated as `(user time + system time) / wall time * 100`;
values above 100% are possible for a multithreaded process. Peak resident
memory is reported in bytes.

GPU utilization is reported as `N/A`. The harness does not have a validated
process-attributable GPU percentage for these GTK3/Cairo workloads. System-wide
GPU readings, including `powermetrics`, are intentionally not substituted.

= Catch2 core microbenchmarks

The Catch2 microbenchmarks measure isolated core operations without starting
GTK or a child process. They complement, but are not directly comparable to,
the process-level measurements above. The benchmark executable is opt-in and
is not registered with CTest:

#raw("cmake -S . -B build/macos-gcc-16 -DTEK4010_BUILD_CATCH2_BENCHMARKS=ON", block: true)
#raw("cmake --build build/macos-gcc-16 --target tek4010-gtk3-core-benchmarks", block: true)
#raw("./build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3/benchmarks/tek4010-gtk3-core-benchmarks", block: true)

The suite covers representative command-line parsing, mapped and unmapped key
translation, and batches of coordinate updates.

The `[plt]` Catch2 cases launch the actual GTK/Cairo executable in
`-performance` mode for each of `test.plt`, `animation.plt`, and the top-level
`pltfiles/*.plt` files. Measurements include process and GTK startup/shutdown
as well as drawing. The windows are visible and close automatically when each
file finishes. In the latest run, `animation.plt` did not finish within 180
seconds. Each Catch2 app launch now has a 30-second timeout; a timeout
terminates the app and reports a failed benchmark case. Cairo initialization
or rendering errors also result in a nonzero app status and fail the case. Run
the finite workloads without it, and consult
`benchmarks/benchmarks.typ` for the measured results and environment:

#raw("./build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3/benchmarks/tek4010-gtk3-core-benchmarks '[plt]~*animation*' --benchmark-samples 3 --benchmark-warmup-time 0", block: true)

The `--plt-files` mode in `run_macos.py` remains available for CSV process
resource measurements of the same files.

= Build and run

From the repository root, build both implementations:

#raw("cmake --build build/macos-gcc-16 --target tek4010 tek4010-gtk3-cpp", block: true)

Run the default demo three times per implementation:

#raw("python3 dev/Tek4010/cpp/tek4010-gtk3/benchmarks/run_macos.py --demo demo --repeats 3 --output build/macos-gcc-16/benchmarks/demo.csv", block: true)

Select more workloads by repeating `--demo`. The finite `fdemo` and `gdemo`
scripts may take longer; each is stopped and reported as timed out if it
exceeds `--timeout-seconds` (default 120). `idemo` loops indefinitely, so it
runs for a fixed 15-second window by default and is then reported as
`duration_limit`:

#raw("python3 dev/Tek4010/cpp/tek4010-gtk3/benchmarks/run_macos.py --demo fdemo --demo gdemo --demo idemo --repeats 3 --timeout-seconds 180 --idemo-seconds 15 --output build/macos-gcc-16/benchmarks/all-demos.csv", block: true)

The script launches both applications with `-exit` and runs them from the
repository root. Their windows are visible during measurement. Avoid interacting
with them and close other resource-intensive applications for more consistent
results. Trials alternate which implementation runs first. Increase the repeat
count and compare medians for more stable comparisons.

The result is CSV. Runs completed by a finite demo have status `completed`;
an `idemo` run stopped at its requested measurement window has status
`duration_limit`. Other non-zero exits, signals, and timeouts are recorded and
cause the script to return a failure status.

= PLT drawing-time benchmarks

The `--plt-files` mode runs the GTK3/C++ executable with `-performance` on
`test.plt`, `animation.plt`, and every top-level `pltfiles/*.plt` file. Each
file is loaded in a fresh emulator process. `wall_seconds` therefore includes
GTK startup and shutdown as well as file drawing; run multiple trials and
compare medians. CSV rows identify the file in the existing `demo` column and
use `cpp-performance` in `implementation`.

#raw("python3 dev/Tek4010/cpp/tek4010-gtk3/benchmarks/run_macos.py --plt-files --repeats 3 --output build/macos-gcc-16/benchmarks/plt-files.csv", block: true)
