#set document(title: "Tek4010 GTK3 Tube Class Code Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 Tube Class Code Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Review

Scope: `tube_` encapsulation in the GTK3 C++ target, including the module interface, implementation, GTK entry point, Tektronix and ARDS consumers, and the test API call.

Outcome: No high-confidence findings. Shared operations are public static methods of `tube`; the line-type and RGB-source drawing helpers are private static methods. The module-global state and the separate legacy C API remain unchanged.

= Findings

No high-confidence findings.

= Validation

- clang-format dry-run passed for the changed C++ files.
- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-tests -j4` passed.
- `ctest --test-dir build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3 --output-on-failure` passed 2/2 tests.
- A targeted search found no remaining `tube_...(` identifiers in C++ target code.
- `git diff --check` passed.

= Remaining risks

Display-backed GTK behavior remains unverified.
