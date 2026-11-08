---
name: update-cpp-test-comments
description: "Review or update C++ test comments in the Tek4010 workspace, including test rationale, setup intent, assertion boundaries, and state transitions."
argument-hint: "C++ test file(s) or fixture(s) to update"
---

# Update C++ Test Comments

## When to Use

- The user asks to review, refactor, or standardize comments in C++ tests.
- The user changes the project's test-comment requirements.
- A C++ test-quality review identifies unclear rationale, phase boundaries, or multi-stage transitions.

## Fork protection

Treat original/upstream files as read-only. Edit only the specific C++ test paths explicitly authorized in the current task and located within an authorized directory. Do not broaden the edit to neighboring tests, implementation, or build/configuration files. If a necessary edit is outside the authorized paths, stop and ask the user before changing it.

## Comment Standard

Use `dev/docs/requirements.adoc` when it defines the test-comment convention. Otherwise follow the user's direction and established conventions in the changed C++ target.

- Begin each test with a concise rationale stating what it verifies, why the behavior matters, and the consequence of an unnoticed failure when consistent with the project convention.
- When setup is meaningful, explain how it establishes the conditions for the behavior under test. Include technical constraints when they materially clarify the scenario.
- Do not invent an Arrange phase for a direct rule/assertion test without meaningful setup.
- Mark meaningful setup, action, and assertion boundaries when useful; explain intent rather than repeat visible calls, values, or expectations.
- Explain distinct state transitions in multi-stage tests when their purpose is not already clear.

## Workflow

1. Follow the requested test-file scope. Do not expand to the full C++ suite unless asked.
2. Read selected tests and enough related implementation/contracts to understand setup, constraints, and observable assertions.
3. Make precise comment-only edits; preserve test behavior and meaningful assertions.
4. Check rationale, phase boundaries, arrangement details, and state transitions for accuracy and redundancy.
5. Run the project's relevant formatter, build, and focused test checks in the caller before delegating to the test-quality reviewer. Include validation results in the task; the reviewer has no shell access and must mark missing evidence as unverified. The caller compiles the report after it is written.
6. Update project handoff/history only when the project maintains those records.

## Completion

Report the files and comment convention changed, validation results, review outcomes, and any explicitly out-of-scope tests that still need attention.
