#set document(title: "Tek4010 GTK3 Architecture Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 Architecture Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Scope and summary

Scope: GTK3 and Cairo boundaries, shared global state, division between host and core functionality, and encapsulation in `dev/Tek4010/cpp/tek4010-gtk3/`. Reviewed the two CMake target definitions and the GTK host, core interface, session/process code, renderer, Tektronix parser, and ARDS implementation.

The GTK boundary is substantially improved: GTK is linked only to the executable, while the core target links Cairo. The host owns GTK lifecycle and supplies Cairo contexts through an explicit host-facing API. Cairo remains intentionally part of the core API. Main remaining concerns are process/resource ownership, mutable singleton state shared across core translation units, and responsibilities concentrated in `tube.cpp`.

= Findings

== High — Process shutdown does not honor process and stream ownership

Location: `src/tube.cpp:650-722`, `src/tube.cpp:741-750`, and `src/tube.cpp:933-938`.

Impact: The PTY stream `getData` is created with `fdopen`, but `tube_quit()` passes it to `pclose`, which is intended for streams returned by `popen` and is not the matching close operation. Shutdown also runs `pkill rs232-console` by process name rather than acting on the stored `childPid`; quitting the emulator can therefore terminate unrelated processes with that name, while the owned child and the other stream do not have clearly scoped cleanup.

Recommendation: Make the terminal process/PTY a single owned session resource. Close `fdopen` streams with `fclose`, and terminate/reap only the child identified by `childPid` when needed. Remove process-name-wide cleanup.

== Medium — Mutable core state is exposed as shared globals

Location: `src/tube_internal.h:20-59`; definitions in `src/tube.cpp:57-132`; parser state in `src/tek4010.cpp:61-80` and `src/ards.cpp:41-57`.

Impact: Session options, terminal dimensions, cursor coordinates, drawing modes, refresh state, and stream handles are shared through process-wide variables. Protocol parsers and render functions mutate this state directly. This hides dependencies, makes reset and isolated testing difficult, and constrains the implementation to one emulator instance.

Recommendation: Introduce an explicit core/session state object and pass it to the session, protocol, and rendering components. Keep implementation state private behind those owning components; do this incrementally to preserve legacy behavior.

== Medium — `tube.cpp` combines several core responsibilities

Location: `src/tube.cpp:417-758`, `src/tube.cpp:776-831`, and `src/tube.cpp:881-913`.

Impact: `tube_init()` parses command-line options, validates fonts, reads the APL key table, creates the PTY, forks/execs the child, and wraps descriptors in streams. The same translation unit implements timing, input polling, process-exit tracking, timer updates, key submission, and host-facing state operations. Changes to process lifecycle or input policy consequently affect a large, globally stateful module.

Recommendation: Separate option parsing from initialization side effects and extract a terminal-session owner for PTY/process/input/output lifecycle. Keep the GTK callback contract small and value-based.

== Low — Protocol interpreters are coupled to both input and Cairo rendering

Location: `src/tek4010.cpp:520-600`, `src/tek4010.cpp:946-953`, `src/ards.cpp:79-117`, and `src/tube_internal.h:61-75`.

Impact: `tek4010_draw()` and `ards_draw()` pull input directly from the session and call Cairo-specific `tube_*` drawing functions while also mutating shared state. The protocol logic is therefore not independently reusable or testable without the terminal input and Cairo renderer. This is consistent with the currently accepted Cairo-specific API, but is a limit on future backends.

Recommendation: Keep the current contract while Cairo is the only intended backend. If a second renderer or parser-only tests become a concrete requirement, separate protocol operations from Cairo execution through a renderer interface or drawing-command stream.

== Low — The protocol core invokes a desktop shell utility directly

Location: `src/tek4010.cpp:308` and `src/tek4010.cpp:1111`.

Impact: The Tektronix parser directly invokes `scrot` through `system()`. This makes a protocol action depend on a desktop utility and shell integration inside the core, complicating portability and isolated tests.

Recommendation: Represent screenshot capture as a host action/capability if this behavior must remain supported across hosts; otherwise document it as a GTK/Linux-specific extension.

= Positive boundaries and accepted decisions

- `src/CMakeLists.txt` places `main.cpp` and `gtk_app.cpp` in the GTK executable and the session, parser, ARDS, and renderer units in `tek4010-gtk3-core`.
- `CMakeLists.txt` links GTK privately to the executable and Cairo publicly to the core. No GTK include or GTK API is used in the reviewed core units.
- `GtkApp` in `gtk_app.cpp` owns the GTK event loop, widgets, callbacks, a `CairoCanvas`, and a `TubeCore` instance; `main.cpp` is the entry point. The core reports timer outcomes through `TubeTimerUpdate` and exposes explicit operations instead of exposing its state to the GTK host.
- The core's Cairo-specific API is an accepted scope choice recorded in design decision D-009, not an accidental GTK dependency.

= Implementation follow-up

The focused fixes were implemented after the initial static review:

- Process/PTY ownership moved into `TerminalSession`, which owns the child PID and both `fdopen` streams, polls/reaps the child, closes streams with `fclose`, and scopes shutdown signals to the owned process group. The process-name-wide `pkill` and mismatched `pclose(getData)` cleanup were removed.
- Core mutable state is owned by a private `TubeCoreState` per `TubeCore` instance. Tek/ARDS parser state, Cairo renderer state, terminal-session ownership, and timing/input state are no longer process-wide globals. Internal core translation units still mutate this aggregate directly, so finer-grained access encapsulation remains possible.
- The parser reports screenshot requests through the core/host API. The GTK host starts `scrot --focused` asynchronously without a shell and reports spawn or non-zero exit failures. The command remains an external desktop utility dependency.
- `tube.cpp` delegates PTY/process lifecycle to `TerminalSession`; command-line parsing, timer policy, and host-facing core operations remain there.
- GTK lifecycle and callback state were extracted into the internal `GtkApp` class in `gtk_app.cpp`; this is the first staged structure change. The target builds and CLI help output matches the captured pre-refactor baseline. Display-backed behavior remains unverified.
- `CairoCanvas` now owns offscreen Cairo surfaces, contexts, composition, viewport scaling, and click mapping; `GtkApp` delegates drawing and coordinate mapping to it.
- `TubeCore` now owns a `TubeCoreState` instance. The GTK host uses its value-based lifecycle, input, timer, and drawing operations; parser and renderer helpers receive the owning state explicitly.
- Cairo-specific protocol/rendering coupling remains intentionally accepted as documented above.

= Validation and limits

The initial architecture review was static. The follow-up C++ target build passed with Homebrew GCC 16. CTest reports no configured tests for this target. GUI lifecycle, screenshot behavior, and display-backed rendering remain unverified.
