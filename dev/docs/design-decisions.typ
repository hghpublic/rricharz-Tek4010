#set document(title: "Tek4010 Design Decisions")
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
  #text(size: 18pt, weight: "bold")[Tek4010 Design Decisions]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= D-001: Keep the C and C++26 applications side by side

*Status:* Accepted

*Rationale:* The legacy C executable is the behavior reference while the GTK3 C++26 port is implemented and validated. Keeping both targets avoids an unapproved cutover and permits direct CLI, protocol, and demo comparisons.

*Evidence:* #link("../../.github/modernize/rearchitecture/spec.md")[Feature specification], #link("../../.github/modernize/rearchitecture/plan.md")[implementation plan], and the unchanged legacy sources under #raw("src/").

*Revisit when:* C++26 parity checks are complete and a separate user-approved change requests a default-target cutover or removal of the C application.

= D-002: Preserve the existing module boundaries for the first port

*Status:* Superseded by D-009

*Rationale:* Port the GTK lifecycle, shared session/rendering code, Tektronix parser, and ARDS parser into corresponding C++ translation units before considering architectural decomposition. This minimizes simultaneous behavior and architecture changes and keeps source-to-target review traceable.

*Evidence:* #link("../../.github/modernize/rearchitecture/artifacts/unit_graph.yaml")[unit graph], #link("../../.github/modernize/rearchitecture/artifacts/units/terminal-emulator/behavior.yaml")[behavior contract], and #link("../../.github/modernize/rearchitecture/artifacts/migration_boundary.yaml")[migration boundary].

*Revisit when:* Parity evidence is available and a focused follow-up identifies a concrete maintenance or testability benefit that justifies restructuring.

= D-003: Use a named module for the C++ API

*Status:* Superseded by the fresh C++26 translation-unit port

*Rationale:* Put the shared terminal API in the `tek4010` primary module interface, compile the tube, Tektronix, and ARDS sources as module implementation units, and have the GTK executable and tests import the module. Keep GTK dependencies on the executable and publish Cairo from the core because its rendering API accepts `cairo_t*`.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cppm")[module interface], #link("../Tek4010/cpp/tek4010-gtk3/src/CMakeLists.txt")[CMake module file set and target dependencies], and #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK module consumer].

*Revisit when:* The target adds another module or changes the GTK/core boundary enough to warrant module partitions or a different public interface.

= D-004: Encapsulate the Tektronix parser routines in `tek4010::cairo::tek4010`

*Status:* Superseded by the fresh C++26 translation-unit port

*Rationale:* Group the six C++ routines previously using the `tek4010_` prefix as static methods of the `tek4010` class. Keep `draw` and `clicked` public for the GTK entry points and tests, and keep parser helpers private. This clarifies the C++ module API without moving shared parser state or changing emulator behavior.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cppm")[class interface], #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cpp")[parser implementation], #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK draw call], and #link("../Tek4010/cpp/tek4010-gtk3/src/tube.cpp")[GTK click call].

*Revisit when:* Parser state is moved out of module globals or instance-based emulator state is introduced.

= D-005: Encapsulate tube operations in `tek4010::cairo::tube`

*Status:* Superseded by the fresh C++26 translation-unit port

*Rationale:* Group the C++ rendering, input, and timing functions previously using the `tube_` prefix as static methods of `tube`. Keep the operations used by the GTK entry point and the Tektronix/ARDS implementation units public, and keep the drawing helpers private. Retain the existing module-global emulator state; introducing instances is a separate architectural change.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cppm")[module API], #link("../Tek4010/cpp/tek4010-gtk3/src/tube.cpp")[tube implementation], and #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK call sites].

*Revisit when:* Emulator state moves from module globals into instances or the GTK/core boundary changes.

= D-006: Encapsulate the ARDS renderer entry point in `tek4010::cairo::ards`

*Status:* Superseded by the fresh C++26 translation-unit port

*Rationale:* Group the C++ `ards_draw` entry point as the public static `draw` method of `ards`. Keep its file-local drawing helpers and parser state internal to the implementation unit; this clarifies the module API without changing ARDS state ownership or rendering behavior.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cppm")[module API], #link("../Tek4010/cpp/tek4010-gtk3/src/ards.cpp")[ARDS implementation], #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK call site], and #link("../Tek4010/cpp/tek4010-gtk3/tests/test_main.cpp")[ARDS test].

*Revisit when:* ARDS exposes additional public operations or its parser state moves to an instance.

= D-007: Separate emulator, Cairo renderer, and GTK host namespaces

*Status:* Superseded by D-009

*Rationale:* Keep parser, process, and protocol state in `tek4010`; group renderer classes, Cairo-facing operations, and drawing state in `tek4010::cairo`; and group GTK callbacks, application state, and lifecycle handling in `tek4010::gtk`. The GTK host owns the event loop and window geometry; `tek4010::cairo::Canvas` owns offscreen surfaces and composition, receiving the host's Cairo draw context and viewport. GTK-specific display/lifecycle options stay in the host, which interacts with core state through host-facing operations. The core target has no GTK API or link dependency.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/src/tek4010.cppm")[namespace API], #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK host], #link("../Tek4010/cpp/tek4010-gtk3/src/tube.cpp")[renderer and emulator implementation], #link("../Tek4010/cpp/tek4010-gtk3/src/ards.cpp")[ARDS renderer], and #link("../Tek4010/cpp/tek4010-gtk3/src/CMakeLists.txt")[target dependencies].

*Revisit when:* Renderer-independent APIs, fully private/instance-owned emulator state, or another application host is introduced.

= D-008: Scope GTK and Cairo build requirements to their consumers

*Status:* Accepted

*Rationale:* Use imported pkg-config targets instead of directory-wide include directories, link directories, and compiler definitions. Link GTK and Cairo usage requirements directly to the legacy C executable, and keep GTK off the C++ core target. This makes the build graph enforce the same GTK/core boundary as the source API.

*Evidence:* #link("../../CMakeLists.txt")[root target dependencies], #link("../Tek4010/cpp/tek4010-gtk3/src/CMakeLists.txt")[C++ target dependencies], and the generated Ninja compile command for `tube.cpp`.

*Revisit when:* The legacy target is removed or its dependency discovery moves to a shared package configuration.

= D-009: Keep GTK host, core session, and Cairo renderer responsibilities separate

*Status:* Accepted

*Rationale:* Keep GTK lifecycle and widget state in the executable host. The host interacts with the terminal core through a small value/action API (`TubeOptions`, `TubeTimerUpdate`, and explicit input/state operations) rather than the core's shared globals. Keep process/session work in `tube.cpp` and Cairo drawing in `tube_render.cpp`. Build these core units as `tek4010-gtk3-core`, link Cairo publicly because the drawing API intentionally uses `cairo_t*`, and keep GTK off the core target. Internal emulator state may remain shared among core translation units in `tube_internal.h`; renderer-independent protocol APIs and instance-owned state are out of scope for this focused increment.

*Evidence:* #link("../Tek4010/cpp/tek4010-gtk3/CMakeLists.txt")[core and GTK target dependencies], #link("../Tek4010/cpp/tek4010-gtk3/src/main.h")[host-facing API], #link("../Tek4010/cpp/tek4010-gtk3/src/main.cpp")[GTK host], #link("../Tek4010/cpp/tek4010-gtk3/src/tube.cpp")[session and timer implementation], #link("../Tek4010/cpp/tek4010-gtk3/src/tube_render.cpp")[Cairo renderer], and #link("../Tek4010/cpp/tek4010-gtk3/src/tube_internal.h")[core-internal shared state].

*Revisit when:* A second application host, a non-Cairo renderer, or focused core-only tests require stronger instance ownership or renderer-independent protocol behavior.
