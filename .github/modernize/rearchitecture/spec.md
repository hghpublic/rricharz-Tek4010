# Feature Specification: Tek4010 GTK3 C++26 Port

> Governed by: `./constitution.md` and `.github/copilot-instructions.md`

**Feature Branch**: `cpp26-gtk3-port`  
**Created**: 2026-11-08  
**Status**: Draft  
**Input**: User description: "migrate tek4010 from c to c++26 in dev/Tek4010/cpp/tek4010-gtk3/CMakeLists.txt"; clarified scope: full-port the GTK3 application.

## Scope Baseline

- **Discovery method**: inspected the root CMake source list and the runtime-reachable C implementation under `src/`, then checked the existing demo scripts.
- **Total items discovered**: 4 C implementation files, 2 C headers, and 4 documented demo scripts.
- **Items in scope**: all runtime behavior in the GTK3 application; implement it in a separate C++26 target under `dev/Tek4010/cpp/tek4010-gtk3/`. The original C sources and root C target remain unchanged as behavioral references.

## User Scenarios & Testing

### User Story 1 — Run an existing Tektronix demo (Priority: P1)

As a Tek4010 user, I can launch the GTK3 emulator with a supported demo script and see the same terminal graphics as before the port.

**Why this priority**: Running existing plot streams is the primary purpose of the emulator and the direct migration acceptance path.

**Independent Test**: Launch each of the four documented demo scripts with the C++ target and compare the resulting behavior with the legacy C application.

**Acceptance Scenarios**:

1. **Given** a usable graphical display, **When** I launch the emulator with `demos/demo.sh`, **Then** the Tektronix window displays the demo without an unexpected error.
2. **Given** a usable graphical display, **When** I launch `demo.sh`, `fdemo.sh`, and `gdemo.sh`, **Then** each finite stream completes with the same visible result and process-exit behavior as the legacy application. When I launch `idemo.sh`, which intentionally loops forever, **Then** its repeated output matches the legacy application until the user closes the emulator.

### User Story 2 — Use the emulator as an interactive terminal (Priority: P1)

As a user, I can launch a command or an interactive shell inside the emulator, send keyboard input, and see terminal output.

**Why this priority**: The PTY and keyboard path is the core interactive use case.

**Independent Test**: Start a short-lived command that prints text and reads a key; verify the child receives the key and exits with the legacy behavior.

**Acceptance Scenarios**:

1. **Given** a command argument, **When** I launch the emulator with that command, **Then** it runs under a PTY with `TERM=tek4014` and its output is rendered.
2. **Given** no command argument, **When** I launch the emulator, **Then** it starts the configured interactive shell, using `/bin/sh` when the current fallback rules apply.
3. **Given** a running child process, **When** I press supported printable, control, Backspace, or Delete keys, **Then** the corresponding terminal input is sent to the child.

### User Story 3 — View Tektronix and ARDS graphics (Priority: P1)

As a user, I can view supported Tektronix 4010/4014 and ARDS streams with their original drawing, cursor, sizing, and clearing behavior.

**Why this priority**: Protocol fidelity and rendering are the emulator's primary externally visible behavior.

**Independent Test**: Replay representative text, vector, point-plot, escape-sequence, GIN, and ARDS inputs and compare state/output with the legacy application.

**Acceptance Scenarios**:

1. **Given** a Tektronix stream, **When** it contains alpha, graphics, point-plot, escape, or incremental-plot controls, **Then** the emulator preserves the legacy interpretation and rendering.
2. **Given** a Tektronix stream that enters GIN mode, **When** the user clicks or presses a key, **Then** the key and encoded coordinates are returned to the child in the established byte order.
3. **Given** the `-ARDS` option and ARDS input, **When** symbols, points, vectors, or clear controls arrive, **Then** the ARDS coordinate and drawing behavior is preserved.

### User Story 4 — Configure display and emulator options (Priority: P2)

As a user, I can use existing options and local controls to select window mode, baud behavior, APL input, and display behavior.

**Why this priority**: These options are established compatibility behavior, though a standard demo can run without most of them.

**Independent Test**: Exercise each documented option and local shortcut, comparing the C++ target with the legacy application.

**Acceptance Scenarios**:

1. **Given** `-h` or `--help`, **When** I launch the emulator, **Then** it prints the existing help and exits successfully.
2. **Given** an unsupported option, too many arguments, or an unavailable command, **When** I launch the emulator, **Then** it emits the corresponding diagnostic and exits nonzero.
3. **Given** `-full`, `-fullv`, `-half`, `-hidecursor`, `-fast`, APL, baud, erase, or wait options, **When** I use the option, **Then** its legacy behavior is retained.

## Edge Cases

- Help is recognized only in the initial option position; the legacy trailing `-noexit` compatibility rule is retained.
- An unknown command, PTY creation/duplication failure, or PTY stream-open failure must not become a success-shaped launch.
- A child that exits must follow the existing interactive-shell, `-exit`/`-noexit`, and `-wait` rules.
- APL mode requires the configured font and may read the user's key translation file.
- ARDS mode ignores mouse GIN input; non-left mouse buttons do not trigger Tek GIN.
- High baud rates automatically enable fast rendering; slow redraw detection is one-way.
- The graphics UI depends on a usable display; headless environments can verify build/CLI behavior but not the visual demo.

## Requirements

### Functional Requirements

- **REQ-001**: The new GTK3 executable MUST be built as C++26 under `dev/Tek4010/cpp/tek4010-gtk3/`, while the existing C application remains unchanged.
- **REQ-002**: The port MUST preserve the existing command-line options, option ordering, help text, diagnostics, and exit outcomes.
- **REQ-003**: The port MUST launch the requested command or fallback shell in a PTY, set `TERM=tek4014`, and preserve child-process and terminal erase configuration behavior.
- **REQ-004**: The port MUST preserve supported baud rates, 7-bit input handling, input pacing, and automatic fast-mode selection for high baud rates.
- **REQ-005**: The port MUST preserve the Tektronix 4010/4014 alpha, graphics, point-plot, escape, ANSI-ignore, incremental, and GIN behavior.
- **REQ-006**: The port MUST preserve keyboard mapping, APL translation/overstrike, break handling, local shortcuts, and GIN response byte ordering.
- **REQ-007**: The port MUST preserve ARDS symbol, point, vector, control-code, coordinate, and clearing behavior.
- **REQ-008**: The port MUST preserve GTK window modes, aspect ratio handling, input signals, timer lifecycle, Cairo surfaces, cursor, character sizing, persistent drawing, and fading/fast rendering.
- **REQ-009**: The port MUST preserve child completion behavior, including interactive-shell shutdown, `-exit`/`-noexit`, and delayed `-wait` termination.
- **REQ-010**: Failures to resolve a command, create/duplicate the PTY, open PTY streams, or create drawing contexts MUST retain a visible diagnostic and nonzero process outcome.

### Assumptions

- The original C executable remains available during and after this port; a cutover/removal is not requested.
- GTK3, Cairo, a graphical display, and POSIX PTY/process APIs remain the runtime environment.
- Existing demo scripts and protocol inputs are the compatibility baseline.
- Visual comparison requires a usable display; automated non-visual checks can cover CLI, protocol, and build behavior.

## Success Criteria

### Measurable Outcomes

- **SC-001**: The new executable builds with the C++26 language mode and links the existing GTK3/Cairo dependencies.
- **SC-002**: All four documented demo scripts run through the new executable without a regression against the legacy C executable; the three finite scripts complete, while `idemo.sh` continues until the user closes the emulator.
- **SC-003**: Every existing CLI option and error path has a corresponding parity check; all checks pass.
- **SC-004**: Representative Tektronix and ARDS streams cover every documented protocol mode and produce the same observable rendering/input behavior as the legacy application.
- **SC-005**: The legacy C target remains buildable and unchanged throughout the port.
