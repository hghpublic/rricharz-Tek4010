#set document(title: "CMake Demo Target Code Review")
#set par(justify: true)
#set page(
  margin: 40mm,
  header: context {
    if here().page() > 1 {
      let page_number = here().page()
      let headings = query(heading.where(level: 1, outlined: true)).filter(
        heading => heading.location().page() <= page_number,
      )
      let current_heading = if headings.len() > 0 {
        box(headings.last().body)
      } else {
        []
      }
      grid(
        columns: (1fr, auto, 1fr),
        gutter: 0pt,
        align: (left, center, right),
        [],
        [#current_heading],
        [#counter(page).display("1")],
      )
    }
  },
  footer: context {
    if here().page() == 1 {
      align(center)[#counter(page).display("1")]
    }
  },
)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[CMake Demo Target Code Review]
]

Date: 2026-11-08

#outline(title: [Contents], depth: 2)

= Review

Scope: the `run_demo` custom target in the root `CMakeLists.txt`.

Outcome: No actionable findings. The target depends on the executable and runs it from the project root, so the relative `demos/demo.sh` argument resolves as expected.

= Findings

No actionable findings.

= Validation

- CMake Tools reconfigured and completed the default build successfully.
- The generated Ninja graph contains `run_demo` and invokes the built `tek4010` executable with `demos/demo.sh` from the repository root.
- The target was not executed because it launches the graphical emulator.

= Remaining risks

- The CMake Tools target list omitted the utility target, although the generated build graph contains it. Runtime behavior was not exercised.
