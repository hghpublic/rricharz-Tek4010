#set document(title: "Tek4010 GTK3 Benchmark Results")
#set page(margin: 22mm)
#set text(font: "Arial", size: 9pt)
#set heading(numbering: "1.")

#align(center)[
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 Benchmarks]
]

= Benchmark types

The Catch2 benchmark executable contains two groups. The `[benchmark]`
cases measure core operations in-process: command-line parsing, key
translation, and coordinate updates. They do not start GTK or render a PLT
file.

The `[plt]` cases launch the GTK3/Cairo application in a fresh process for
each sample, using `-performance -exit cat <file>`. The elapsed time includes
process creation, GTK startup, PLT input and rendering, and application
shutdown. Each app launch is limited to 30 seconds; a timeout terminates and
reaps the app process, then fails that Catch2 case instead of blocking the
suite indefinitely. Windows are visible while each case runs. The tests cover
`test.plt`, `animation.plt`, and the top-level `pltfiles/*.plt` files.
Canvas initialization and rendering failures produce a nonzero application
exit status, so Catch2 reports them as failed cases rather than successful
timings.

To run the core cases:

#raw("./build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3/benchmarks/tek4010-gtk3-core-benchmarks '[benchmark]'", block: true)

To run the finite PLT workloads, excluding `animation.plt`:

#raw("./build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3/benchmarks/tek4010-gtk3-core-benchmarks '[plt]~*animation*' --benchmark-samples 3 --benchmark-warmup-time 0", block: true)

The `run_macos.py --plt-files` mode is a separate macOS process-resource
harness. It records wall time, CPU use, and peak resident memory as CSV; those
measurements should not be compared directly with Catch2 timings.

= Results

The following Catch2 results were collected on 2026-11-10. Each completed
workload used one sample and one iteration, so these are indicative single-run
wall times, not stable performance estimates. Repeat the run and compare
medians when measuring changes.

These timings predate the change that disables stroke-deflection delays in
`-performance` mode. Re-run the finite workload command above to measure the
current implementation before using the values as a baseline.

#table(
  columns: (2fr, 1fr),
  table.header([Workload], [Elapsed]),
  [test.plt], [267.887 ms],
  [animation.plt], [Not completed; see note],
  [pltfiles/01_barchart.plt], [239.723 ms],
  [pltfiles/02-das-dot.plt], [246.206 ms],
  [pltfiles/03_pretty016.plt], [251.544 ms],
  [pltfiles/04_slinky.plt], [217.588 ms],
  [pltfiles/05_snoopy.plt], [210.971 ms],
  [pltfiles/06_ssws.plt], [304.899 ms],
  [pltfiles/07_usmap.plt], [228.104 ms],
  [pltfiles/08_isco36.plt], [376.281 ms],
  [pltfiles/09_wizard.plt], [220.280 ms],
  [pltfiles/10_wizidd.plt], [221.836 ms],
  [pltfiles/11_wizwarp.plt], [233.708 ms],
  [pltfiles/12-ekg4.plt], [284.341 ms],
  [pltfiles/13_bugss14.plt], [330.095 ms],
  [pltfiles/14_fudge.plt], [553.420 ms],
  [pltfiles/15_bostonufo.plt], [823.522 ms],
  [pltfiles/16_gplanet.plt], [474.844 ms],
)

#strong[Note.] The full `[plt]` run completed eight workloads before reaching
`animation.plt`; the benchmark was still running after 180 seconds and was
stopped. The remaining finite workloads were measured separately with
`animation.plt` excluded. This is not a completed timing for the animation.

= Environment

- macOS 27.0.1, arm64, Apple M4 Max
- Homebrew GCC 16.2.0
- Catch2 3.16.1
- CMake build directory: `build/macos-gcc-16`; `CMAKE_BUILD_TYPE` was unset

Results depend on machine load, GTK/Cairo state, and desktop activity. The
single-sample values above are a snapshot for this environment, not a
performance guarantee.
