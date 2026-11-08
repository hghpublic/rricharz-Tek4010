#set document(title: "Tek4010 GTK3 C++ Constant Conversion Code Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 C++ Constant Conversion Code Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Review

Scope: Replacing object-like constant macros in the C++26 project under `dev/Tek4010/cpp/tek4010-gtk3/` with `static constexpr` declarations where appropriate. Platform/compiler feature macros and conditional-compilation guards were intentionally retained.

Outcome: The C++ Code Quality Reviewer found no actionable correctness or behavior-preservation issue in the constant conversions. The unused `DEBUG` and `DEBUGMAX` definitions in `tube.cpp` were removed; `DEBUG` in `tek4010.cpp` remains a compile-time false constant used by existing debug branches.

= Findings

No actionable findings.

= Validation

- clang-format 23.1.5 formatting check passed for all six modified C++ source/header files.
- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-cpp` passed.
- `git diff --check` passed.
- Search confirmed that remaining `#define` directives are feature-test definitions; remaining conditional directives are platform/API guards.

= Remaining risks

GUI behavior was not tested. The build continues to emit the existing duplicate `-lstdc++` linker warning.
