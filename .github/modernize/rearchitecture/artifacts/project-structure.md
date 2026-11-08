# Project Structure

## Project type

Desktop GTK application and command-line-launched terminal emulator. The repository also contains a separate experimental C++ application area under `dev/Tek4010/cpp/`.

## Runtime structure

- The root CMake project builds the legacy `tek4010` C executable from `src/`.
- `src/main.c` owns process startup, GTK window creation, event callbacks, drawing surfaces, and keyboard mapping.
- `src/tube.c` owns shared emulator state, option parsing, PTY/child-process setup and lifecycle, input timing, and Cairo primitives.
- `src/tek4010.c` implements the Tektronix 4010/4014 byte-stream state machine and graphics behavior.
- `src/ards.c` implements ARDS-mode byte-stream handling.
- `src/main.h` and `src/tube.h` declare cross-file entrypoints and shared state.
- `dev/Tek4010/CMakeLists.txt` includes the C++ experiments; `dev/Tek4010/cpp/tek4010-gtk3/` is the declared destination for the GTK3 C++26 port and currently contains empty CMakeLists files.

## Entry points and functional areas

1. `main(argc, argv)`: parses startup through `tube_init`, initializes GTK, configures the display, and enters the GTK event loop.
2. GTK draw/key/mouse/destroy callbacks: drive redraw, keyboard-to-PTY input, GIN coordinate input, and shutdown.
3. GTK timer callback: checks child status, completion delay, and redraw/input readiness.
4. Tektronix protocol parser: handles alpha, graphics, point-plot, escape, ANSI-ignore, incremental, and GIN states.
5. ARDS protocol parser: handles symbols, points, vectors, and control codes.
6. PTY session: launches the requested command or an interactive shell and relays terminal bytes.

## Layers

- Presentation: GTK3 window, event callbacks, Cairo surfaces.
- Terminal/session: CLI flags, pseudo-terminal, child process, input pacing, and keyboard forwarding.
- Emulation: Tektronix 4010/4014 and ARDS state machines.
- Rendering: Cairo text, vectors, points, cursors, fading, and character sizing.

The current source is a compact cross-file C program rather than a packaged multi-layer architecture. Most state is global and declared in `tube.c`/`tube.h`.

## Configuration and build

- `CMakeLists.txt`: builds the legacy C application, resolves GTK3/Cairo with pkg-config, and adds `dev/Tek4010`.
- `Makefile`: builds the C application with C99, GTK3 pkg-config flags, and generates `src/help.h` from the help text.
- `dev/vcpkg/`: supplies the workspace triplet/toolchain and macOS dependency overlays.
- `dev/Tek4010/cpp/`: currently contains GTK3 and SDL3 C++ subproject placeholders.
