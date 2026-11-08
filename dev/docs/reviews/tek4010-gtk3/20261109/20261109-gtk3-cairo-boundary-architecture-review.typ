#set document(title: "Tek4010 GTK3 and Cairo Boundary Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 and Cairo Boundary Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Scope and summary

Scope: Architectural boundaries among the GTK3 application, the C++ emulator/rendering module, and Cairo under `dev/Tek4010/cpp/tek4010-gtk3/`.

The GTK application owns widgets, signals, the GTK main loop, Cairo surface/context creation, composition, and context lifetime. Rendering entry points receive caller-owned `cairo_t*` contexts, and Cairo image-surface tests can exercise them without a GTK display. Two boundaries remain coupled: the tube layer controls GTK lifecycle, and the GTK application directly reads and mutates exported emulator state. Cairo itself is intentionally part of the module API rather than hidden behind a renderer abstraction.

= Findings

== Medium — Tube code controls GTK lifecycle

Location: `src/tube.cpp:48`, `src/tube.cpp:822`, `src/tube.cpp:829`, `src/tube.cpp:842`, and `src/module_globals.hpp:6`.

Impact: `tube.cpp` includes GTK and `tube::onTimerEvent` calls `gtk_main_quit()` directly, as well as `main_finished()`, a callback implemented by the GTK application to update the window title. This couples the emulator/timer layer to GTK lifecycle and makes the core target require GTK even though the public module interface exposes no GTK types. The dependency is also published from the core target in `src/CMakeLists.txt:17`.

Recommendation: Have the timer operation return a narrow outcome (for example, redraw, finished, or quit) or use an injected host callback. Let the GTK adapter perform `gtk_main_quit()` and window-title updates. Then keep GTK out of the core module's public link interface.

== Medium — GTK code accesses mutable emulator globals

Location: `src/tek4010.cppm:35`, `src/tek4010.cppm:46`, `src/tek4010.cppm:67`, `src/tek4010.cppm:77`, and `src/main.cpp:306`.

Impact: The module exports mutable parser/configuration state, and the UI reads and writes it directly. For example, the GTK key handler sets `tube_doClearPersistent`; other UI code changes `argFast` and `aplMode` and accesses `putKeys`. This makes the exported global list an implicit bidirectional contract between the UI and emulator rather than a narrow application API.

Recommendation: Gradually replace UI access with explicit commands and queries on an application-facing facade, such as requesting a clear, selecting fast rendering, changing APL mode, and sending input. Keep parser state private to the emulator implementation.

== Low — Cairo is a public rendering contract

Location: `src/tek4010.cppm:5`, `src/tek4010.cppm:83`, `src/tek4010.cppm:127`, and `src/CMakeLists.txt:17`.

Impact: The module interface includes Cairo and all drawing entry points accept raw `cairo_t*`; the core target also publishes Cairo as a dependency. This makes the core renderer Cairo-specific. It is not a GTK leak, and is a reasonable boundary if Cairo is the intended fixed rendering backend.

Recommendation: Keep this contract if Cairo is a deliberate product constraint. If backend substitution or a renderer-independent core becomes a requirement, introduce a rendering interface/command boundary and keep Cairo implementations behind it.

= Existing boundary strengths

- `main.cpp` owns GTK callbacks and Cairo surfaces/contexts, and dispatches rendering to `tek4010::draw` or `ards::draw`.
- Core draw operations receive contexts from the caller; `main.cpp` destroys its temporary contexts after drawing.
- `tests/test_main.cpp` uses Cairo image surfaces and does not require GTK widgets or a display.
- `module_globals.hpp` is a small bridge containing only window dimensions/name and the completion callback; the public C++ module interface itself does not include GTK headers or types.

= Validation and limits

This was a static architecture review of the current worktree. No files outside the review record were changed. No build, test, or display-backed GUI run was performed for this review.
