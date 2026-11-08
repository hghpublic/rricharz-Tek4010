# Project Guidelines

## Fork preservation and change boundaries
- Treat original/upstream project files as read-only. Modify or create files only inside directories or at individual file paths explicitly authorized in the current task.
- Typical project-specific customization areas are `dev/`, `.github/`, and `.vscode/`; the root `CMakeLists.txt` may also be changed when explicitly requested. These examples do not authorize changes elsewhere or unrelated edits within those paths.
- If a required change falls outside the authorized paths, stop and ask the user to approve that path before editing. Do not modify, delete, or rename original files to make a task easier.

## Code and behavior
- The emulator is written in C with GTK 3 and Cairo. Use C99 and follow the surrounding source style.
- Preserve terminal emulation behavior and supported input/file formats when changing the parser or display code.
- `src/help.h` is generated from `src/help.txt` by the Makefile; edit the text source, not the generated header.
- Keep platform and dependency changes consistent across the Makefile, CMake configuration, vcpkg manifest, and platform guides.
- CMake includes `dev/vcpkg/configure.cmake` before `project()`. Preserve this ordering for vcpkg triplet and compiler configuration.

## Build and test
- Use `make` to build and `make check` for the graphical demo smoke test; the check requires a usable display.
- See [README.md](../README.md) and the platform guides for installation requirements.

## Handoff and chat history
- At the start of each chat, read `dev/copilot/chats/tek4010/tek4010-handoff.md`, `dev/docs/requirements.adoc`, and the current date's history at `dev/copilot/chats/tek4010/session-history/session-history-YYYYMMDD.md` when they exist.
- At the end of each chat, update or create the handoff with the current status, decisions, blockers, and next useful action. Append a concise, factual summary to the current date's history; create the file when needed.
- Do not record secrets or unrelated personal information.

## Setup and project updates
- When asked to create a new project using this workflow, first read `dev/copilot/setup/setup.adoc` and any applicable companion guide under `dev/copilot/setup/`. Reuse the setup process, not this project's product scope or design decisions.
- Update `dev/docs/requirements.adoc` when project scope, assumptions, priorities, or acceptance criteria change. Keep requirements consistent with demonstrated behavior.
- Keep repository paths rooted at the workspace root; documentation and review reports belong under `dev/docs/`, and Copilot handoff/setup material belongs under `dev/copilot/`.

## Project development and reviewers
- Develop in small increments that can be built and tested where practical. Preserve existing terminal-emulation behavior and supported input/file formats.
- Keep C code C99-compatible and follow the existing GTK 3/Cairo boundaries. For C++ work, inspect the target's CMake configuration and follow its declared language standard and local conventions; do not assume the C++ target shares the C application's implementation details.
- After each implementation increment, perform an appropriate code-quality review. When tests are added or changed, also perform a test-quality review. Use the C++ reviewer agents for C++ changes; for C or other targets, use an available target-appropriate reviewer or complete a focused review directly.
- The C++ reviewer agents have no shell access. Run relevant formatting, build, and test checks in the caller before delegation, include their results and exact scope in the task, and mark missing evidence as not run or unverified. Do not have a reviewer request or wait for shell execution permission.
- Address actionable findings, or record a concise rationale and remaining risk for any accepted deferral.

## Review reports and background reviews
- Record every code-quality and test-quality review, including a direct review, as a standalone Typst source report. Use `dev/docs/reviews/<target>/YYYYMMDD/` for one target and `dev/docs/reviews/shared/YYYYMMDD/` when multiple targets are materially in scope. Use `YYYYMMDD-<scope>-code-review.typ` or `YYYYMMDD-<scope>-test-review.typ`; add a numeric suffix rather than overwrite an existing report.
- Include scope, outcome, findings or an explicit no-findings statement, validation evidence, and remaining risks. Findings must identify location, impact, and a suggested action. Compile reports with `typst compile` when available; keep generated PDFs out of the repository unless requested.
- Time-box each background review to five minutes from launch. After at most 180 seconds without a usable result, send one targeted follow-up. At five minutes, complete the scoped review directly, write the report, and record the fallback and remaining limitations in the handoff/history. Tool-call activity does not reset the deadline; do not wait for cancellation acknowledgement. Continue independent work while the review runs.

## Design decisions and Typst
- Maintain `dev/docs/design-decisions.typ` as the design-decision register. Give each decision a stable ID, status, rationale, evidence/links, and revisit condition. Update it when a decision changes or a design document is added; keep links and statuses current.
- All project-authored Typst documents, including design documents and review reports, use `#set par(justify: true)`, `#set page(margin: 40mm)`, `#set text(font: "Arial", size: 10pt)`, and `#set heading(numbering: "1.1")`.
- Place `#outline(title: [Contents], depth: 2)` after the title and metadata and before the main sections.
- Omit the header on page one and center its Arabic page number in the footer. On later pages, select headings with `heading.where(level: 1, outlined: true)`, keep only those whose `heading.location().page() <= here().page()`, center the latest one without wrapping when it fits, and put the page number at the right. Use equal flexible side columns around a content-sized center column; keep the page number visible even when no outlined heading has appeared.
