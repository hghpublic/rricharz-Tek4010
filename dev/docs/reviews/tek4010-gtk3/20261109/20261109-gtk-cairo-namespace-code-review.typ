#set document(title: "Tek4010 GTK and Cairo Namespace Code Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK and Cairo Namespace Code Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Review

Scope: `tek4010.cppm`, `main.cpp`, `tube.cpp`, `tek4010.cpp`, `ards.cpp`, and the GTK3 source `CMakeLists.txt`, including deletion of `module_globals.hpp`. The refactor places parser/process/protocol state in `tek4010`, renderer APIs and state in `tek4010::cairo`, and GTK application callbacks, state, and timer/lifecycle handling in `tek4010::gtk`.

Outcome: No actionable, high-confidence findings. The code review found the namespace organization, timer relocation, module exports, and GTK/Cairo target dependencies consistent with the requested boundary. GTK is linked to the executable; Cairo remains public because renderer entry points accept `cairo_t*`.

= Findings

No actionable, high-confidence findings.

= Validation

- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-tests -j 4` passed.
- `ctest --test-dir build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3 --output-on-failure` passed 2/2 tests.
- `clang-format --dry-run --Werror` passed for the changed C++ files.
- `git diff --check` passed.
- A focused search found no GTK includes or GTK calls in the Cairo/core implementation units.

= Remaining risks

Display-backed GTK behavior remains unverified. The review was read-only and did not independently run the build or tests.
