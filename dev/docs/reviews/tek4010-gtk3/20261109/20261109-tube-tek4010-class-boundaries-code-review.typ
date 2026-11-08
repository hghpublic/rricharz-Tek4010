#set document(title: "Tek4010 Tube and Tek4010 Class Boundary Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 Tube and Tek4010 Class Boundary Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Review

Scope: `tek4010::cairo::tube` and `tek4010::cairo::tek4010`, their state and implementations, their renderer/parser callers in `ards.cpp`, and focused tests. This was a read-only architectural and logical-boundary review.

Outcome: The classes are static-method groupings rather than state-owning objects. `tek4010` is largely cohesive as the Tek protocol interpreter, but its frame loop mixes input scheduling, protocol state transitions, and rendering. `tube` has a clearer separation problem: process/session, terminal input, timing, event dispatch, and Cairo drawing primitives all share one class. Extract responsibilities in phases; do not mechanically move individual methods into more static classes.

= Findings

== Medium — `tube` combines session control with CRT rendering

Location: `src/tek4010.cppm:126-150`, `src/tube.cpp:243-413`, `src/tube.cpp:429-744`, and `src/tube.cpp:747-1260`.

Impact: One class exposes timekeeping and baud pacing, input readiness and reads, key translation, command-line parsing, PTY/process creation and cleanup, mouse dispatch, and Cairo drawing operations. These responsibilities have different reasons to change and different dependencies. In particular, `init` owns both emulator option parsing and child-process startup, `quit` performs process cleanup from the renderer API, and rendering primitives share state with the input/session code. This makes the class difficult to test independently and encourages callers to depend on a broad static API.

Recommendation: Split first at the responsibility boundary, not by method count:

- Introduce a `PtySession` or `TerminalSession` that owns PTY/process lifetime, input/output streams, readiness, and byte/key translation.
- Keep Cairo-specific CRT behavior in a stateful `TubeRenderer` or `PhosphorDisplay` that owns viewport, cursor, phosphor/fade, line style, and drawing state.
- Keep frame cadence in the existing GTK/Cairo integration for now; avoid a separate timing class until the scheduler has a concrete independent consumer.

Preserve the current bounded input-per-frame behavior while moving session ownership. Move click routing and process cleanup out of the rendering class as part of that boundary.

== Medium — shutdown is not scoped to the emulator's child process

Location: `src/tube.cpp:669`, `src/tube.cpp:695`, and `src/tube.cpp:739-744`.

Impact: `tube::init` records the spawned child in `childPid` and wraps the PTY descriptor with `fdopen`, but `tube::quit` passes that stream to `pclose` (which is for `popen` streams) and then runs `pkill rs232-console`. The stream cleanup does not match how it was created, while the name-based kill can terminate unrelated `rs232-console` processes. Shutdown is not scoped to the child/session this emulator owns.

Recommendation: Have the session owner shut down only its recorded child/process group and close its own descriptors. Preserve any required serial-console cleanup through an explicit owned-process handle rather than a global name-based kill.

== Medium — `tek4010` is not yet an encapsulated interpreter object

Location: `src/tek4010.cppm:28-29,114-124`, `src/tek4010.cpp:52-84`, and `src/tek4010.cpp:252-1157`.

Impact: The class methods are static and operate on module-level protocol state (`mode`, `savemode`, coordinate bytes, GIN state, and other renderer globals). The large `draw` routine polls and paces input, applies protocol transitions, dispatches rendering, and controls cursor/frame behavior. Creating another interpreter instance would not isolate its state, and the class boundary does not prevent tests or other consumers from modifying that state directly.

Recommendation: Keep Tek protocol interpretation as one cohesive responsibility, but make it an instance that owns a `Tek4010State`. In a later step, separate byte consumption/state transitions from frame pumping and drawing by having the interpreter emit operations to the CRT renderer. Keep escape-sequence handling and coordinate decoding with the interpreter unless a separately testable protocol component emerges; do not extract helpers solely because `draw` is long.

== Low — `tube` owns Tek-specific input dispatch

Location: `src/tube.cpp:721-735`.

Impact: `tube::clicked` is a mouse-event adapter that special-cases ARDS and calls `tek4010::clicked` for GIN. This reverses the intended dependency direction: a shared CRT renderer knows which protocol implementation is active. The ARDS parser and Tek interpreter otherwise both use `tube` drawing primitives.

Recommendation: Route mouse input through the active emulator/interpreter from the host or a small emulator coordinator. Leave coordinate-to-display rendering behavior in the selected protocol object, not in `TubeRenderer`.

= Suggested target structure

- `PtySession`: process and terminal byte transport.
- `TubeRenderer`: persistent/temporary Cairo drawing and CRT state shared by Tek and ARDS.
- `Tek4010Interpreter`: Tek protocol state and transitions.
- `ArdsInterpreter`: ARDS protocol state and transitions.
- A thin GTK/Cairo frame coordinator: poll a bounded amount of input, let the selected interpreter update state/render, then composite.

This is a destination, not a recommendation to introduce all five types in one change. The first implementation increment should isolate session ownership and add tests around session behavior; the next should move Tek protocol state into an interpreter instance. Keep the existing behavior and output under parity tests at each step.

= Positive boundaries and remaining risks

- `tek4010.cpp` and `ards.cpp` already keep protocol-specific logic in separate implementation units, while both share the same CRT drawing operations.
- `ards.cpp` has file-local protocol state and delegates drawing to `tube`; it provides a useful precedent for keeping protocol state separate, though it also remains static and is not instance-isolated.
- The test suite exercises selected GIN, Tek alpha, and ARDS behavior, but the inspected tests also set module globals directly. It does not establish independent-instance behavior or comprehensively cover session lifecycle, mouse dispatch, and escape modes.
- This review made no code changes and ran no build or tests. It found architectural risks, not a demonstrated runtime regression.
