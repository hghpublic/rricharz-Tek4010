---
name: update-cpp-code-comments
description: "Review, clarify, or update C++ implementation comments in the Tek4010 workspace, including source-file purpose, API contracts, design intent, and logical code groups."
argument-hint: "C++ source/header file(s) or modules to update"
---

# Update C++ Code Comments

## When to Use

- The user asks to review, clarify, refactor, or standardize comments in C++ implementation code.
- A C++ code-quality review identifies unclear design intent, undocumented constraints, or redundant comments.
- The project changes its C++ source-comment convention.

## Scope

This skill covers C++ implementation source and headers, not tests. For C++ test code, including test fixtures and helpers, use `update-cpp-test-comments`.

## Fork protection

Treat original/upstream files as read-only. Edit only the specific C++ source/header paths explicitly authorized in the current task and located within an authorized directory. Do not broaden the edit to neighboring files or alter build/configuration files. If a necessary edit is outside the authorized paths, stop and ask the user before changing it.

## Comment Standard

Use `dev/docs/requirements.adoc` when it defines the comment convention. Otherwise follow the user's direction and established conventions in the changed C++ target.

- Begin each implementation source file or translation unit in scope with a concise design comment explaining its responsibility and architectural boundaries.
- Give each non-trivial class, struct, or enum a purpose comment that explains its role, key invariant, or place in the implementation.
- Add comments at meaningful logical blocks so a reader can follow what each phase does, how it connects to surrounding control or data flow, and why the implementation is structured that way. Comments should tell the design story across the file, not just label code.
- Add purpose or contract comments to callable items when their role, invariant, or ownership/lifetime behavior is not clear from the code.
- Explain non-obvious intent or invariants at meaningful boundaries such as state transitions, resource acquisition/cleanup, error paths, and C/C++ or GTK/Cairo integration.
- For public APIs, describe caller-relevant behavior, guarantees, ownership/lifetime expectations, and constraints.
- Keep comments accurate, human-readable, and close to the code they explain. Explain design intent rather than narrating statements; do not repeat nearby comments or obvious code, and do not force comments onto trivial code.

## Workflow

1. Follow the user's requested file and module scope.
2. Read the selected C++ source/header files and enough related declarations, call sites, and project documentation to understand intent, ownership, error handling, and architecture.
3. Inventory in-scope source files, types, and callable items. Check file/type purpose coverage and inspect meaningful logical blocks for design context.
4. Make precise comment edits. Preserve executable behavior; change code structure only when requested.
5. Check that comments tell a coherent what/how/why story, remain accurate and well placed, avoid repetition, and follow the project's documented convention.
6. Run the project's relevant formatting and validation checks. If delegating review, provide the results to the reviewer; reviewers in this workspace have no shell access.
7. Update project handoff/history only when the project maintains those records.

## Completion

Report the files and comment intent changed, validation results, review outcomes, and any explicitly out-of-scope files that still need attention.
