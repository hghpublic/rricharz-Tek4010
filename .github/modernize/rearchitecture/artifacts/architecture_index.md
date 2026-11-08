# Tek4010 C++26 Port — Implementation Index

## Implementation Guide

This index is not the full contract. Do not implement from this file alone; follow the artifact paths below.

### Global artifacts

- `unit_graph.yaml`
  - Who reads: all implementation workers.
  - Use for: runtime boundary, CLI entrypoint, GTK callback entrypoints, and source anchors.
  - How to consume: read the `terminal-emulator` unit and its callback wiring.
- `migration_boundary.yaml`
  - Who reads: all implementation workers.
  - Use for: exact port scope and permitted legacy residue.
  - How to consume: implement only `must_rewrite`; leave original `src/*.c` unchanged.
- `wire_contracts.yaml`
  - Who reads: workers touching command input or process/PTY behavior.
  - Use for: external stream and command-line behavior.
  - How to consume: preserve the recorded 7-bit terminal byte behavior and command invocation.
- `shared_modules.yaml`
  - Who reads: all implementation workers.
  - Use for: modules shared across multiple runtime units.
  - How to consume: currently empty because the application is analyzed as one runtime unit.
- `cross_unit_state.yaml`
  - Who reads: all implementation workers.
  - Use for: state crossing runtime-unit boundaries.
  - How to consume: currently empty; all analyzed state is within the application unit.

### Unit: `terminal-emulator`

- External trigger: CLI invocation, followed by GTK events and PTY input.
- Must read:
  - `units/terminal-emulator/behavior.yaml` — preserve CLI branches, child-process lifecycle, terminal parsing, rendering, and event behavior.
  - `units/terminal-emulator/bindings.yaml` — preserve GTK signal/timer wiring.
  - `units/terminal-emulator/unit_decomposition.yaml` — inspect optional split candidates only; target structure remains a design choice.
- Relevant global rows:
  - `wire_contracts.yaml` — rows for `terminal-emulator`.
  - `shared_modules.yaml` — rows where `used_by_units` includes `terminal-emulator`.
  - `cross_unit_state.yaml` — flows whose writer or reader is `terminal-emulator`.
- Before DONE report: artifact paths read, implemented `must_preserve` items, unresolved contracts, and build/runtime evidence.
