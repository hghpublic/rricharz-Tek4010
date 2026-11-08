#set document(title: "Tek4010 GTK3 C++26 Test Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 C++26 Test Review]
]

Date: 2026-11-08

#outline(title: [Contents], depth: 2)

= Review

Scope: `dev/Tek4010/cpp/tek4010-gtk3/tests/` and its CMake test registration, including the unit tests, CLI parity test, and build dependencies.

Outcome: The build-order finding and a low-severity assertion-scope finding were fixed. The tests target now depends on both executables used by CLI parity. The ARDS clear test checks five surface locations, and the GIN test checks two coordinate cases.

= Finding disposition

#table(
  columns: (auto, auto, 1fr, 1fr),
  [*Severity*],
  [*Location*],
  [*Finding and impact*],
  [*Disposition*],
  [P2],
  [#raw("tests/CMakeLists.txt:13")],
  [The test target depended on the legacy executable only, although CLI parity also invokes the C++ executable. A clean or incremental build of the test target could therefore fail or compare against a stale C++ binary.],
  [Added `tek4010-gtk3-cpp` to `add_dependencies(tek4010-gtk3-tests ...)`; rebuilt the target and reran CTest successfully.],
  [Low],
  [#raw("tests/test_main.cpp"), ARDS form-feed clear assertion],
  [The clear test initially checked only the first surface pixel, so a partially cleared display could still pass.],
  [Expanded the assertion to sample all four corners and the center before and after the clear; rebuilt and reran CTest successfully.],
)

= Coverage and remaining risks

- Automated cases cover an unmapped key, GIN response ordering and suppression state at two coordinates, Tek alpha cursor advancement, ARDS form-feed pixel clearing and short-vector coordinates, and CLI parity for help, an unsupported option, and too many arguments.
- Mapped-key translation, PTY lifecycle/failure paths, other Tek modes, and ARDS symbol/point/extended-vector modes remain untested. These are not claimed as covered.
- GTK rendering, mouse interaction, and demo parity were not exercised because no graphical display was available.

= Validation

- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-tests -j4` passed after the expanded GIN and ARDS assertions.
- `ctest --test-dir build/macos-gcc-16/dev/Tek4010/cpp/tek4010-gtk3 --output-on-failure` passed 2/2 registered tests.
