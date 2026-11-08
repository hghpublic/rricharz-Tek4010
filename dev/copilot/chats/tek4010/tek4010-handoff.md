# Tek4010 Copilot Handoff

Updated: 2026-11-09

## Current state

- The upstream C application remains under `src/`. The fresh C++26 port is under `dev/Tek4010/cpp/tek4010-gtk3/`.
- The C++ project builds a GTK host executable and a `tek4010-gtk3-core` static library. GTK and Cairo are linked only to the host; the core receives a renderer-neutral `TubeRenderer` interface.
- `main.cpp` is a thin entry point; `GtkApp` in `gtk_app.cpp` owns GTK lifecycle, callbacks, window state, and one `TubeCore` instance. `CairoCanvas` in `cairo_canvas.cpp` owns the offscreen surfaces, drawing contexts, aspect-preserving viewport composition, and click-coordinate mapping.
- `TubeCore` in `main.h` and `tube.cpp` owns a private `TubeCoreState` per instance. Its options, input/key-translation and throttling, diagnostics, clock, and process/request state are grouped behind private fields and operation APIs. Tek and ARDS parser state is private to their parser-owned types and operations. There is no process-wide mutable core singleton.
- `TubeRenderer` in `tube_renderer.h` defines the backend-neutral drawing contract. `GtkApp` owns a `CairoTubeRenderer`, binds the per-frame Cairo contexts, and passes it to `TubeCore`; `tube_render.cpp` implements the Cairo backend outside the core library.
- `TerminalSession` owns the PTY child, streams, terminal input/output, polling, and scoped process cleanup. Renderer-specific font and point-history state now belongs to the Cairo renderer.
- Window dimensions and character-cell geometry are private to `TubeDisplayGeometry`; protocol positions and margins are private to `TubeCoordinates`. The former shared `TubeState` aggregate has been removed.
- Startup and rendering mode options (ARDS, APL enablement, auto-clear, tab expansion, character-size retention, and fast mode) are private to `TubeCoreOptions` and accessed through option operations.
- Persistent-clear requests and bright-spot frame countdowns are private to `TubeRenderLifecycle`; both parsers use its clear-request query, and drawing/timer paths use its lifecycle operations.
- Cursor visibility is private to `TubeCursorState`; Tek and ARDS parsers use explicit show/hide operations, and cursor rendering queries its visibility.
- The active APL character-set mode is private to `TubeCharacterSetState`; the Tek parser and core use its operations, distinct from the startup option that enables APL support.
- Tek point-plot, special-plot, and write-through modes are private to `Tek4010State`; clearing the persistent display resets plot modes through a parser operation, and the renderer receives write-through mode through the render-state snapshot.
- Tek intensity and defocus controls are private to `Tek4010State`; clear resets them through a parser operation, and the renderer receives their values through the render-state snapshot.
- The shared draw-time budget is private to `TubeFrameTiming`; unused `refreshCount` and `isBrightSpot` fields were removed after confirming there are no readers.
- Pen size and line type are private to `TubeStrokeStyle`; core and parser updates use its operations, while the renderer receives their values through the state snapshot.
- Current and target coordinates, margins, movement, and clear reset are encapsulated by `TubeCoordinates`; the renderer receives coordinate values through the state snapshot.
- `TubeCoreState` now privately owns its state components and exposes typed mutable and const accessors, making the component boundary explicit while preserving operation-based access.
- `TubeCoreOptions::parseCommandLine()` owns argument parsing and option updates; `TubeCore::initialize()` delegates APL input setup and terminal-session startup.
- State-component methods and command-line parsing are in `tube_state.cpp`; renderer-neutral drawing and render-state operations are in `tube_render_operations.cpp`; clock, input polling, and APL input setup are in `tube_runtime.cpp`. `tube.cpp` retains core initialization, timer orchestration, and host-facing operations.
- Tek GIN input mode and echoed-character suppression are owned by `Tek4010State`; `TubeCore` delegates key capture to the parser, and the GIN fields are no longer part of shared `TubeState`.
- Tektronix screenshot operations request a host action. The GTK host launches `scrot --focused` asynchronously without a shell and reports launch and process failures.
- The C++ source/header files in this target are formatted with clang-format; the current formatter check passes.
- Constant-like object macros in the C++ target were replaced with `static constexpr` declarations where applicable; platform and feature-test preprocessing directives remain.
- The constant-conversion changes passed clang-format verification, the C++ target build, and the focused code-quality review.
- The protocol implementations remain in the migrated C++ translation units. Renderer-independent drawing APIs are established; further renderer extraction remains deferred until a second backend is in scope.
- Preserve the manually chosen CMake `FOLDER` properties in the C++ project's root `CMakeLists.txt`.

## Validation

- `cmake --build build/macos-gcc-16 --target tek4010-gtk3-cpp` passed.
- The Cairo canvas extraction rebuilt successfully; the executable's `--help` command completed successfully, clang-format and `git diff --check` passed, and CTest reported that no tests are configured.
- The per-instance core-state extraction rebuilt successfully; the independent-instance CTest passed 1/1, the executable's `--help` command completed successfully, and clang-format and `git diff --check` passed.
- Tek parser state was encapsulated behind parser-owned operations; the core and executable rebuilt successfully, the core-instance CTest passed 1/1, and formatting and whitespace checks passed.
- Window and character-grid geometry was moved behind `TubeDisplayGeometry`; the core and executable rebuilt successfully, its geometry regression check is covered by the core-instance CTest, and the executable's `--help` check passed.
- Legacy mode flags were moved from `TubeState` to `TubeCoreOptions`; the core-instance test covers option defaults and mutation, and build, formatting, and whitespace checks passed.
- Persistent-clear and bright-spot state was moved into `TubeRenderLifecycle`; the core-instance test covers request/complete and frame-count transitions, and build, CTest, `--help`, formatting, and whitespace checks passed.
- GIN mode and echo-suppression tracking were moved into `Tek4010State`; the core and executable rebuilt, the core-instance CTest passed, and `--help`, formatting, and whitespace checks passed.
- Cursor visibility was moved into `TubeCursorState`; the core-instance test covers its default and show/hide transitions, and build, CTest, `--help`, formatting, and whitespace checks passed.
- Dynamic APL character-set mode was moved into `TubeCharacterSetState`; the core-instance test covers its default and transitions, and build, CTest, `--help`, formatting, and whitespace checks passed.
- Tek point-plot and special-plot flags were moved into `Tek4010State`; the core and executable rebuilt, the core-instance CTest passed, and `--help`, formatting, and whitespace checks passed.
- Tek write-through mode was moved into `Tek4010State`; the renderer-neutral snapshot reads it through a public accessor, and build, CTest, `--help`, formatting, and whitespace checks passed.
- Tek intensity and defocus controls were moved into `Tek4010State`; the core and executable rebuilt, the core-instance CTest passed, and `--help`, formatting, and whitespace checks passed.
- Draw-time budget access was moved into `TubeFrameTiming`, and unused refresh state was removed; the core-instance test covers the timing API, and build, CTest, `--help`, formatting, and whitespace checks passed.
- Pen size and line type were moved into `TubeStrokeStyle`; the core-instance test covers default and updated values, and build, CTest, `--help`, formatting, and whitespace checks passed.
- Coordinate fields were removed from the shared state aggregate and moved into `TubeCoordinates`; tests cover movement, endpoint copying, margins, and clear reset, and build, CTest, `--help`, formatting, and whitespace checks passed.
- `TubeCoreState` component composition was made private behind typed accessors; the core and executable rebuilt, the core-instance CTest passed, and `--help`, formatting, and whitespace checks passed.
- Command-line parsing was moved onto `TubeCoreOptions`; a focused test covers option updates and the first command argument, the core and executable rebuilt, CTest, `--help`, invalid-option exit behavior, formatting, and whitespace checks passed.
- State-component implementations and command-line parsing were extracted into `tube_state.cpp` and registered with the core target; build, CTest, CLI checks, formatting, and whitespace checks passed.
- Renderer-neutral drawing and render-state operations were extracted into `tube_render_operations.cpp` and registered with the core target; build, CTest, CLI checks, formatting, and whitespace checks passed.
- Clock, input polling, and APL input setup were extracted into `tube_runtime.cpp` and registered with the core target; build, CTest, CLI checks, formatting, and whitespace checks passed.
- C and C++ `--help` and unsupported-option outputs and exit statuses matched.
- Core compile/link commands were checked to confirm they do not depend on GTK; `git diff --check` passed.
- The focused formatting review found no actionable issue in the inspected sources, but could not compare the before-and-after diff; see the dated review report for this limitation.
- GUI behavior was not exercised. The linker emits a duplicate `-lstdc++` warning.

## Follow-up and known risks

- Keep `TubeCore::state_` as `std::unique_ptr<TubeCoreState>` for now. This is not required for per-instance state ownership; a value member would avoid allocation and indirection but require the complete internal state definition in `main.h`, widening header dependencies. Revisit whether that header isolation is worth the extra pointer and out-of-line destructor.
- The architecture review and implementation follow-up are recorded in `dev/docs/reviews/tek4010-gtk3/20261109/20261109-gtk3-architecture-review.typ`. Process ownership, screenshot integration, GTK hosting, Cairo composition, per-instance core state, and the renderer-neutral interface are implemented.
- The renderer-neutral boundary, runtime-state operation APIs, private Tek/ARDS parser state, encapsulated window/character geometry, coordinates, mode options, render lifecycle, Tek GIN interaction state, cursor visibility, dynamic APL mode, Tek plot modes, Tek intensity/defocus controls, private draw timing, and stroke style are in place; a future SDL3/WebGPU implementation remains out of scope. Shared state now consists of operation-based components owned by `TubeCoreState`.
- The core-instance test verifies that fast-mode changes in one `TubeCore` do not affect another; CTest passes 1/1.
- The core no longer depends on Cairo. `scrot` remains an external GTK-host dependency.
- Process shutdown and screenshot behavior have not been exercised interactively; GUI/display behavior remains unverified.
- Keep this handoff current when additional changes or validation alter the state above; use the dated session history for chronological detail.
