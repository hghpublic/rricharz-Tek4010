#set document(title: "Tek4010 GTK Core Boundary Code Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK Core Boundary Code Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Review

Scope: The focused C++26 refactor under `dev/Tek4010/cpp/tek4010-gtk3/`, including GTK host/core responsibilities, timer outcomes, host-facing operations, Cairo rendering extraction, target dependencies, and preservation of configured CMake `FOLDER` values.

Outcome: No actionable findings. The GTK host owns the UI lifecycle, the core exposes host operations without requiring the host to access shared globals, session/input code is separated from Cairo drawing, and GTK is not a dependency of the core target. The focused design intentionally retains Cairo as the rendering API.

= Findings

No actionable correctness, behavior-preservation, or build-boundary findings were identified by the focused C++ code-quality review.

= Validation

- Caller validation: `cmake --build build/macos-gcc-16 --target tek4010-gtk3-cpp` passed.
- Caller validation: C and C++ `--help` and unsupported-option outputs and exit statuses matched.
- Caller validation: core compile/link commands were confirmed to have no GTK dependency; `git diff --check` passed.
- Reviewer confirmed the report scope and found no high-confidence issue in the reviewed C++ target.

= Remaining risks

Display-backed rendering and interactive GTK lifecycle behavior were not exercised. The core retains shared internal state in `tube_internal.h`, and protocol rendering remains Cairo-bound by design. The linker reports a duplicate `-lstdc++` warning.
