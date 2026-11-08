#set document(title: "Tek4010 GTK3 C++26 Code Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 C++26 Code Review]
]

Date: 2026-11-08

#outline(title: [Contents], depth: 2)

= Review

Scope: `dev/Tek4010/cpp/tek4010-gtk3/` CMake integration, shared headers, four C++ translation units, and target boundary.

Outcome: One review observation was resolved without changing runtime behavior. The reviewer found that the copied automatic-fast-mode comment described three slow redraw events, while the implementation and behavior contract use two. The C++ comment now accurately states two events; the preserved threshold remains unchanged.

= Finding disposition

#table(
  columns: (auto, auto, 1fr, 1fr),
  [*Severity*],
  [*Location*],
  [*Review observation*],
  [*Disposition*],
  [Medium],
  [#raw("src/main.cpp"), automatic-fast-mode comment],
  [Comment claimed a threshold of three events, but the code switches at #raw("slowCount >= 2").],
  [Corrected the C++ comment to match the legacy implementation and behavior contract, both of which specify two events. No behavior change.],
)

= Validation

- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-cpp tek4010-gtk3-tests -j4` passed.
- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-tests -j4` passed after adding both executable dependencies to the test target.
- `ctest --test-dir build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3 --output-on-failure` passed both registered tests after the latest test additions.
- The target build includes the legacy C executable and the C++26 executable required by the CLI parity test.

= Remaining risks

The review did not have display-backed evidence. GTK rendering, mouse interaction, and the full demo set remain unverified because `DISPLAY` and `WAYLAND_DISPLAY` were unset. Automated parser coverage includes Tek alpha cursor advancement, GIN response behavior, ARDS form-feed clearing, and a short vector; other Tek/ARDS modes remain unverified.
