# Implementation Plan: Tek4010 GTK3 C++26 Port

**Date**: 2026-11-08 | **Spec**: [spec.md](./spec.md)

## Summary

Build a separate C++26 GTK3/Cairo executable under `dev/Tek4010/cpp/tek4010-gtk3/`, preserving the existing C application as an unchanged behavior reference. Port the CLI and PTY lifecycle, Tektronix and ARDS parsers, and GTK/Cairo event and rendering paths; retain the existing command-line and byte-stream contracts. Add focused CTest/Catch2 checks and compare the new executable with the legacy one wherever behavior can be checked without a display.

## Technical Context

**Language/Version**: C++26; enable CXX in the GTK3 subproject without changing the root project's C language configuration.  
**Primary Dependencies**: GTK3, Cairo, POSIX PTY/process APIs, and Catch2 (already declared in the vcpkg manifest).  
**Storage**: None; the application has no database or persisted application data.  
**Testing**: CMake/CTest with Catch2, deterministic CLI and protocol checks, side-by-side legacy parity checks, and display-backed demo smoke tests.  
**Target Platform**: macOS and other supported POSIX hosts with GTK3, Cairo, PTY support, and a usable graphical display for GUI validation.  
**Performance Goals**: Preserve the existing 35 ms timer cadence, byte pacing, drawing behavior, and fast-mode transitions.  
**Constraints**: Keep `src/` unchanged; confine implementation to `dev/Tek4010/cpp/tek4010-gtk3/`; preserve supported CLI, PTY, Tektronix, ARDS, GTK, and Cairo behavior; GUI verification needs a display.

## Constitution Check

1. **Keep the existing C application unchanged and buildable** — PASS. All conversion work targets the new GTK3 subproject.
2. **Implement only under `dev/Tek4010/cpp/tek4010-gtk3/`** — PASS. The plan does not require edits to legacy source or root CMake configuration.
3. **Use C++26 and preserve GTK3/Cairo/POSIX and user-visible behavior** — PASS. The target will set C++26 explicitly and use the existing dependency stack and contracts.
4. **Preserve demo scripts and supported streams; avoid unrelated redesign** — PASS. Demo scripts and legacy behavior artifacts are the parity baseline.
5. **Validate increments and record review evidence and limitations** — PASS. Each module receives focused checks; final build, CTest, GUI smoke, review reports, and handoff updates are planned.

## Applied Guidelines

No C-to-C++ migration guideline matching this GTK3/Cairo application was present in `skills/guidelines/`. The plan therefore follows the repository's behavior-preservation constitution, the architecture artifacts, and the explicit source anchors and contracts in the legacy implementation.

## Implementation Steps

### Step 1: Configure the isolated C++26 target
- **Requirements**: REQ-001, REQ-010
- **Design inputs**: [constitution.md](./constitution.md), [tech-stack.md](./artifacts/tech-stack.md), [migration_boundary.yaml](./artifacts/migration_boundary.yaml)
- **Description**: Establish a C++26 executable and source subdirectory in the designated GTK3 project. Resolve GTK3 and Cairo at target scope, keep the root C target and its language configuration unchanged, and expose an unambiguous demo target for the new executable.

### Step 2: Define C++ interfaces and generated help input
- **Requirements**: REQ-001, REQ-002
- **Design inputs**: [unit_graph.yaml](./artifacts/unit_graph.yaml), [behavior.yaml](./artifacts/units/terminal-emulator/behavior.yaml)
- **Description**: Port the shared declarations and state needed by the four implementation modules into C++ headers under the new target. Make the target consume a build-generated help header copied from the tracked help source, without depending on or modifying the legacy generated header.

### Step 3: Port CLI, PTY, and child lifecycle
- **Requirements**: REQ-002, REQ-003, REQ-004, REQ-009, REQ-010
- **Design inputs**: [behavior.yaml](./artifacts/units/terminal-emulator/behavior.yaml), [wire_contracts.yaml](./artifacts/wire_contracts.yaml), [cross_unit_state.yaml](./artifacts/cross_unit_state.yaml)
- **Description**: Preserve argument parsing and diagnostics, shell fallback, command resolution, PTY setup, `TERM`, erase configuration, 7-bit input handling, baud pacing, high-baud fast mode, child monitoring, and exit/wait behavior. Keep failures explicit and nonzero.

### Step 4: Port terminal protocol parsers
- **Requirements**: REQ-005, REQ-006, REQ-007
- **Design inputs**: [behavior.yaml](./artifacts/units/terminal-emulator/behavior.yaml), [wire_contracts.yaml](./artifacts/wire_contracts.yaml), [unit_decomposition.yaml](./artifacts/units/terminal-emulator/unit_decomposition.yaml)
- **Description**: Preserve the Tektronix 4010/4014 parser states and GIN response encoding, plus the ARDS parser's coordinate decoding, control handling, and drawing calls. Keep the existing shared drawing boundary unless a focused build or parity check demonstrates that a small interface adjustment is needed.

### Step 5: Port GTK/Cairo lifecycle, input, and rendering
- **Requirements**: REQ-005, REQ-006, REQ-008, REQ-009
- **Design inputs**: [bindings.yaml](./artifacts/units/terminal-emulator/bindings.yaml), [behavior.yaml](./artifacts/units/terminal-emulator/behavior.yaml), [project-structure.md](./artifacts/project-structure.md)
- **Description**: Preserve GTK signal and timer registration, window modes and aspect ratio, Cairo surfaces and drawing behavior, cursor and character sizing, redraw/fast-mode behavior, keyboard/APL handling, mouse GIN routing, and shutdown callbacks.

### Step 6: Add behavior-parity checks
- **Requirements**: REQ-002, REQ-003, REQ-004, REQ-005, REQ-006, REQ-007, REQ-009, REQ-010
- **Design inputs**: [spec.md](./spec.md), [behavior.yaml](./artifacts/units/terminal-emulator/behavior.yaml), [wire_contracts.yaml](./artifacts/wire_contracts.yaml), `tektests/gintest.c`
- **Description**: Add CTest/Catch2 coverage for deterministic CLI, PTY, protocol, and failure behavior, and compare the new target with the legacy executable for the same inputs where practical. Retain `tektests/gintest.c` as a manual GIN interoperability fixture; it is not currently registered as an automated test.

### Step 7: Validate, review, and hand off
- **Requirements**: REQ-001 through REQ-010
- **Design inputs**: all artifacts listed above and the acceptance criteria in [spec.md](./spec.md)
- **Description**: Build the new target in C++26, run CTest and all four demo scripts against the new and legacy executables, and perform display-backed smoke checks when a graphical display is available. Obtain the required C++ code and test quality reviews, record them as Typst reports, and update the project handoff and session history with evidence and remaining limits.

## Project Structure

```text
dev/Tek4010/cpp/tek4010-gtk3/
├── CMakeLists.txt
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── tube.cpp
│   ├── tek4010.cpp
│   ├── ards.cpp
│   ├── main.hpp
│   ├── tube.hpp
│   └── help.h                 # generated in the build tree
└── tests/
    ├── CMakeLists.txt
    └── ...                    # CTest/Catch2 unit and parity tests
```

## Testing Strategy

- **appType**: Native desktop application (GTK3/Cairo), with a CLI and PTY-backed child process.
- **Critical user journeys**:
  1. `-h`/`--help`, supported options, unknown options/commands, too-many-arguments, and their exact diagnostics and exit outcomes.
  2. Launch a deterministic command in a PTY, verify `TERM=tek4014`, terminal input/output, and the expected child-exit behavior.
  3. Run `demo.sh`, `fdemo.sh`, and `gdemo.sh` with both executables and compare completion and visible results; compare a bounded display session of the intentionally infinite `idemo.sh`.
  4. Replay Tek alpha/graphics/point/escape/incremental/GIN inputs; verify key and coordinate response byte order and keyboard/APL behavior.
  5. Replay ARDS symbol/point/vector/control inputs and verify coordinate, clear, and drawing behavior.
- **primaryValidationStack**: CMake/Ninja C++26 build; CTest/Catch2 tests against real local POSIX PTYs and deterministic inputs; side-by-side CLI/protocol checks against the legacy C executable; display-backed execution of the four demo scripts.
- **fallbackMatrix**:
  - `infra-tier`: Not applicable; there is no database, message broker, cache, or external service. PTY and child-process behavior use the host POSIX implementation.
  - `browser-tier`: Not applicable; this is a native GTK desktop application, not a web application. If a display is unavailable, retain build, CLI, PTY, and protocol checks and report GUI rendering as unverified rather than substituting browser tests.
- **Environment requirements**: CMake and Ninja; a C++26-capable compiler; GTK3 and Cairo from the existing vcpkg setup; POSIX PTY/process APIs; Catch2 from the manifest; and a usable display for GUI demo verification. Docker is unavailable in the planning environment and is not required. Node.js 24.18.0 is available but is not required.
- **knownGaps**: Automated screenshot comparison and GUI event replay are not established in the current repository. If no usable display is available, visual rendering, mouse interaction, and live keyboard behavior remain unverified. `tektests/gintest.c` is a manual fixture, not an automated suite.
- **Test data strategy**: Reuse checked-in demo streams and deterministic protocol byte fixtures; launch isolated short-lived child commands; wait for and reap each child; avoid assumptions about user shell state or pre-existing files; no database or persistent test data is involved.
- **Acceptance criteria**:
  - CMake builds the new executable in C++26 and the existing C target remains buildable and unchanged.
  - CLI output, diagnostics, and exit outcomes match the legacy executable for covered options and failures.
  - Unit/parity checks pass for PTY setup, byte handling, GIN encoding, Tek and ARDS protocol behavior, and child completion.
  - All four demos run without unexpected errors; the three finite scripts complete and the infinite `idemo.sh` is closed by the user after a bounded visual comparison.
- **External dependency strategy**: Link to real GTK3/Cairo; exercise real host PTYs and child processes using deterministic local commands. There are no remote services or protocol boundaries requiring mocks. Catch2 is already in the vcpkg manifest and is the test framework; do not add another test framework.
- **Test infrastructure**: Register focused module tests through CTest/Catch2, with deterministic fixtures for CLI, PTY, Tek, and ARDS behavior. Each implementation increment runs its relevant module tests and reports pass/fail/skip counts; a failure blocks completion of that increment.
- **Legacy Test Assets**:
  - `legacyTestAssets`: `tektests/gintest.c`, a manually driven GIN protocol helper.
  - `source`: discovered.
  - `migrationDecision`: reuse-with-edits.
  - `migrationRationale`: Retain it as a manual interoperability fixture if required by the new build; it is not a legacy automated suite and should not be represented as automated coverage.
  - `legacyTestMappingTable`: `tektests/gintest.c` → manual GIN interoperability check → GIN key/coordinate byte contract.
- **Validation review expectations**: Review the exact changed C++ implementation and test files with the repository's C++ quality reviewers after validation. Include build/test evidence, GUI limitations, and any accepted findings in standalone Typst reports; do not claim unrun display checks as passing.

## Implementation Phases and Tasks

### Phase 1: Setup

- [x] T001 [Plan:1.1] Configure the isolated C++26 executable and source/test subdirectories in `dev/Tek4010/cpp/tek4010-gtk3/CMakeLists.txt`, `dev/Tek4010/cpp/tek4010-gtk3/src/CMakeLists.txt`, and `dev/Tek4010/cpp/tek4010-gtk3/tests/CMakeLists.txt`; link GTK3/Cairo and add a uniquely named demo target.

### Phase 2: Foundational

- [x] T002 [Plan:2.1] Port shared application declarations and state into `dev/Tek4010/cpp/tek4010-gtk3/src/main.hpp` and `dev/Tek4010/cpp/tek4010-gtk3/src/tube.hpp`, and configure a build-tree help header from `src/help.h.txt` [Source: src/main.h] [Source: src/tube.h].
- [x] T003 [Plan:6.1] Add CTest/Catch2 test registration and module test scaffolding under `dev/Tek4010/cpp/tek4010-gtk3/tests/`, keeping tests independent of a graphical display where possible.

### Phase 3: User Story 2 — Use the emulator as an interactive terminal (Priority: P1)

- [ ] T004 [US2] [Plan:3.1,6.1] Port CLI parsing, PTY creation, child setup/lifecycle, input pacing, and shared Cairo primitives into `dev/Tek4010/cpp/tek4010-gtk3/src/tube.cpp`; add focused CLI, PTY, and failure-path tests [Source: src/tube.c].

### Phase 4: User Story 3 — View Tektronix and ARDS graphics (Priority: P1)

- [ ] T005 [US3] [Plan:4.1,6.1] Port the Tektronix 4010/4014 state machine and GIN response handling into `dev/Tek4010/cpp/tek4010-gtk3/src/tek4010.cpp`; add representative protocol and byte-order tests [Source: src/tek4010.c].
- [ ] T006 [US3] [Plan:4.1,6.1] Port ARDS parsing, coordinate decoding, and drawing dispatch into `dev/Tek4010/cpp/tek4010-gtk3/src/ards.cpp`; add representative symbol, point, vector, and control-code tests [Source: src/ards.c].

### Phase 5: User Story 1 — Run an existing Tektronix demo (Priority: P1)

- [ ] T007 [US1] [Plan:5.1,6.1] Port GTK startup, window and aspect-ratio handling, signal/timer callbacks, Cairo surfaces, drawing dispatch, and mouse/keyboard event routing into `dev/Tek4010/cpp/tek4010-gtk3/src/main.cpp`; add a display-independent startup check and document the display-backed smoke command [Source: src/main.c].

### Phase 6: User Story 4 — Configure display and emulator options (Priority: P2)

- [ ] T008 [US4] [Plan:3.1,5.1,6.1] Add option and local-control parity cases for window modes, baud/fast behavior, APL translation/overstrike, break handling, shortcuts, cursor options, and exit/wait behavior under `dev/Tek4010/cpp/tek4010-gtk3/tests/`.

### Phase 7: Validation and handoff

- [x] T009 [Plan:6.1,7.1] Build both executables and run the current CTest suite, which checks CLI parity plus GIN coordinates, Tek alpha cursor advancement, ARDS form-feed pixel clearing, and short-vector coordinates; record exact commands and pass/fail/skip counts.
- [ ] T010 [US1] [Plan:5.1,7.1] Run `demo.sh`, `fdemo.sh`, and `gdemo.sh` to completion with both executables; compare a bounded `idemo.sh` session and close each emulator using the same user action, recording any display-related limitation.
- [x] T011 [Plan:7.1] Complete C++ code-quality and test-quality reviews for the changed implementation and tests, then write and compile the required Typst reports under `dev/docs/reviews/tek4010-gtk3/20261108/`.
- [x] T012 [Plan:7.1] Update `dev/copilot/chats/tek4010/tek4010-handoff.md`, append the implementation outcome to `dev/copilot/chats/tek4010/session-history/session-history-20261108.md`, and update `dev/docs/design-decisions.typ` with the port boundary decision.

## Execution Status

T001-T003 and T009-T012 are complete. T004-T008 remain open because broad PTY/CLI failure coverage, all parser modes, keyboard mapping, and display-backed startup/rendering checks are incomplete. T010 is blocked until a graphical session is available; no visual demo parity is claimed. The latest build and CTest commands passed, with 2/2 registered tests passing.

## Requirement Mapping

| REQ ID | Description | Plan Items | Implementation Evidence |
|--------|-------------|------------|------------------------|
| REQ-001 | Add an isolated C++26 GTK3 target and preserve the C target | 1.1, 2.1, 7.1 | `dev/Tek4010/cpp/tek4010-gtk3/CMakeLists.txt`, `src/CMakeLists.txt`; C and C++ build evidence |
| REQ-002 | Preserve CLI options, ordering, help, diagnostics, and exits | 3.1, 6.1 | `src/tube.cpp`; CLI parity tests |
| REQ-003 | Preserve PTY command/shell launch, TERM, and erase behavior | 3.1, 6.1 | `src/tube.cpp`; PTY child tests |
| REQ-004 | Preserve baud rates, 7-bit input, pacing, and fast-mode selection | 3.1, 6.1 | `src/tube.cpp`; byte/pacing tests |
| REQ-005 | Preserve Tektronix parser modes and GIN behavior | 4.1, 5.1, 6.1 | `src/tek4010.cpp`, `src/main.cpp`; protocol and GIN tests |
| REQ-006 | Preserve keyboard/APL/break/shortcut and GIN response behavior | 4.1, 5.1, 6.1 | `src/tek4010.cpp`, `src/main.cpp`; input and response tests |
| REQ-007 | Preserve ARDS parsing and drawing behavior | 4.1, 6.1 | `src/ards.cpp`; ARDS protocol tests |
| REQ-008 | Preserve GTK modes, signals, Cairo surfaces, cursor, sizing, and rendering | 5.1, 7.1 | `src/main.cpp`, `src/tube.cpp`; build and display-backed demo evidence |
| REQ-009 | Preserve child completion, `-exit`/`-noexit`, and `-wait` behavior | 3.1, 5.1, 6.1 | `src/tube.cpp`, `src/main.cpp`; child lifecycle tests |
| REQ-010 | Preserve diagnostics and nonzero outcomes for launch/drawing failures | 1.1, 3.1, 5.1, 6.1 | `src/tube.cpp`, `src/main.cpp`; failure-path tests |
