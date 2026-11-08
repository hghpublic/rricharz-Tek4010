#set document(title: "Tek4010 GTK3 and Cairo Boundary Follow-up Review")
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
  #text(size: 18pt, weight: "bold")[Tek4010 GTK3 and Cairo Boundary Follow-up Review]
]

Date: 2026-11-09

#outline(title: [Contents], depth: 2)

= Scope and summary

Scope: GTK3 and Cairo architectural boundaries in `dev/Tek4010/cpp/tek4010-gtk3/`, including the GTK host, named-module API, renderer implementations, and CMake target wiring.

The previous boundary review's primary finding is resolved: GTK event-loop and window operations now live in `tek4010::gtk`, the renderer implementation units contain no GTK calls or GTK headers, and GTK is linked to the executable rather than the core target. This review identified GTK-specific configuration and direct host access to public mutable state as remaining coupling, plus root CMake GTK compile-setting leakage. The follow-up implementation now addresses those host/build boundaries; raw module state remains exported for existing core/test consumers. Cairo is deliberately visible in the renderer API.

= Follow-up implementation status

- #strong[GTK configuration:] fullscreen, half-screen, cursor, keyboard, wait, and exit options are parsed in the GTK host before core initialization. The host selects the title from mode flags while preserving those flags for core initialization; core no longer exports or writes GTK-owned options.
- #strong[Host/core interface:] GTK uses `tek4010::host` operations for process status and input, and `tek4010::cairo` operations for animation, fast mode, and clear requests. Direct GTK reads/writes of process, APL/GIN, stream, and renderer globals have been removed. The module still exports low-level mutable state for existing core/test consumers; fully hiding or instance-owning that state remains a separate change.
- #strong[Cairo composition:] `tek4010::cairo::Canvas` now owns offscreen surface creation, renderer dispatch, and composition. GTK retains window geometry and supplies the draw context, viewport dimensions, and offsets.
- #strong[Build configuration:] the legacy C target now links imported GTK/Cairo pkg-config targets directly; root directory-wide include, link, and definition settings are removed. The generated core compile command contains Cairo includes but no GTK include path.

= Findings

== Medium — Core initialization owns GTK-specific configuration

Location: `src/tek4010.cppm:37-50` and `src/tube.cpp:440-595`.

Impact: The module exports `tek4010::gtk` window-title and display/lifecycle options, and `tek4010::cairo::tube::init` parses command-line options and writes those GTK-owned values. The code has no GTK API dependency, but the core initialization contract still depends on the GTK host's namespace and mixes emulator setup with window policy. A second host or a headless renderer would inherit GTK concepts even though GTK is linked only to the executable.

Status: Resolved. GTK display/lifecycle options are parsed in the host, and core initialization no longer refers to `tek4010::gtk`.

Recommendation: Parse host/display options in the GTK layer and pass a small host-neutral emulator configuration to core initialization. Keep window title, fullscreen, half-screen, cursor visibility, and wait/exit policy in a GTK-owned configuration object; have the host select the title from parsed mode information instead of having core code mutate a GTK namespace.

== Medium — GTK handlers use mutable renderer and emulator globals directly

Location: `src/main.cpp:168-218`, `src/main.cpp:250-340`, `src/main.cpp:346-468`, and the exported state in `src/tek4010.cppm:23-35` and `src/tek4010.cppm:73-91`.

Impact: The GTK timer polls and updates process state, reads renderer state, and controls exit behavior. The draw and key callbacks also set renderer flags, change APL/GIN state, and write directly to the emulator's input stream. Namespaces classify this state but do not define a narrow host/core contract; changes to emulator internals can therefore require edits throughout the GTK adapter.

Status: Partially resolved. GTK now uses host/rendering operations instead of direct mutable state. The exported low-level state remains available to other module consumers and tests.

Recommendation: Introduce a small application-facing emulator interface with operations and queries such as `poll()`, `sendKey()`, `requestClear()`, `setAplMode()`, `isFinished()`, and a redraw/status result. Keep process streams, parser state, and renderer globals private to their owning implementation. Add tests around the interface before moving the legacy state.

== Low — Root CMake directory settings leak GTK compile configuration

Location: root `CMakeLists.txt:22-24`; the GTK3 core target is declared in `dev/Tek4010/cpp/tek4010-gtk3/src/CMakeLists.txt:1-29`.

Impact: Root-level `include_directories(${GTK3_INCLUDE_DIRS})` and `add_definitions(${GTK3_CFLAGS_OTHER})` apply to targets in the directory subtree, including `tek4010-gtk3-core`. The generated Ninja compile command for `tube.cpp` contains GTK 3 include paths even though the core target no longer links `PkgConfig::GTK3`. This weakens the build-time boundary and can allow accidental GTK header use to compile unnoticed.

Status: Resolved. GTK and Cairo usage requirements are attached to the legacy executable through imported pkg-config targets; GTK paths are absent from the core compile command.

Recommendation: Scope the legacy C executable's GTK/Cairo include paths, definitions, and link directories to its target, or use imported pkg-config targets and target-scoped usage requirements. Keep `PkgConfig::GTK3` on the GTK executable and Cairo on the renderer/core target.

== Low — Cairo is part of the public module contract

Location: `src/tek4010.cppm:4-5`, `src/tek4010.cppm:52-141`, and `src/CMakeLists.txt:17-20`.

Impact: The module interface includes Cairo and its renderer entry points accept raw `cairo_t*`; consequently, Cairo is a public dependency of every module consumer. This is appropriate for a deliberately Cairo-specific renderer, but it means the emulator/rendering API is not backend-independent.

Status: Intentionally unchanged. `cairo::Canvas` consolidates Cairo resource ownership and composition, but the renderer API remains Cairo-specific.

Recommendation: Retain this contract while Cairo is the fixed backend. If another backend or a renderer-independent core becomes a real requirement, introduce a neutral drawing-command or renderer interface rather than adding abstraction preemptively.

= Opportunities that preserve the current boundary

- Keep `main()` global and the GTK callbacks in `tek4010::gtk`; this is the correct host direction.
- The GTK callback now computes its viewport and delegates surface ownership, renderer dispatch, and composition to `cairo::Canvas`.
- Preserve GTK-free renderer translation units and display-independent Cairo image-surface tests. These are valuable boundary checks.
- Avoid moving Cairo surface ownership into GTK-independent core unless there is a concrete host or backend need; the GTK adapter is currently the natural owner of the GTK-provided draw context.

= Validation and limits

The follow-up implementation built with Homebrew GCC 16.2.0; CTest passed 2/2, including CLI parity. The generated core compile command was checked and contains no GTK include path. `git diff --check` passed. Display-backed GTK behavior remains unverified.
