# GTK3 C++ Module Dependencies and Callgraph

These diagrams summarize the CMake target boundary and the main runtime call paths in the C++26 GTK3 emulator.

## Module Dependencies

<!-- mermaid-checked: no \n, no em-dash/en-dash, no {} in labels, subgraphs are id["label"], arrows are -->|"label"|, all subgraphs closed by end, ids unique -->
~~~mermaid
flowchart TD
    subgraph HostTarget["GTK host target"]
        HostExe["tek4010-gtk3-cpp"]
        MainSource["main.cpp"]
        GtkAppSource["gtk_app.cpp"]
        CanvasSource["cairo_canvas.cpp"]
        RenderSource["tube_render.cpp"]
    end
    subgraph CoreTarget["Core static library"]
        CoreLib["tek4010-gtk3-core"]
        TubeModule["TubeCore in tube.cpp"]
        CoreState["TubeCoreState in tube_internal.h"]
        TekModule["tek4010.cpp"]
        ArdsModule["ards.cpp"]
        SessionModule["terminal_session.cpp"]
        InternalHeader["tube_internal.h"]
        RendererContract["tube_renderer.h"]
    end
    subgraph Runtime["Libraries and runtime"]
        Gtk["GTK3"]
        Cairo["Cairo"]
        PtyChild["PTY child process"]
        Scrot["scrot utility"]
    end

    HostExe -->|"contains"| MainSource
    HostExe -->|"contains"| GtkAppSource
    HostExe -->|"contains"| CanvasSource
    HostExe -->|"links"| CoreLib
    HostExe -->|"private dependency"| Gtk
    CoreLib -->|"contains"| TubeModule
    CoreLib -->|"contains"| CoreState
    CoreLib -->|"contains"| TekModule
    CoreLib -->|"contains"| ArdsModule
    CoreLib -->|"contains"| SessionModule
    CoreLib -->|"contains"| InternalHeader
    CoreLib -->|"contains"| RendererContract
    HostExe -->|"contains"| RenderSource
    HostExe -->|"private dependency"| Cairo
    TubeModule -->|"owns instance state and session"| CoreState
    SessionModule -->|"creates and communicates with"| PtyChild
    GtkAppSource -->|"spawns on request"| Scrot
~~~

### Technology Stack Summary

| Layer | Technology | Version | Purpose |
|---|---|---|---|
| Host | C++ | C++26 | GTK application and emulator host |
| Host | GTK | GTK3, resolved by CMake | Window lifecycle, events, and drawing surface |
| Core | C++ | C++26 | Emulator protocol and terminal-session implementation |
| Rendering | Cairo | Resolved by CMake | Host-owned rendering backend and canvas composition |
| Terminal I/O | POSIX PTY | Platform-provided | Interactive child process input and output |
| Screenshot | scrot | Runtime utility | Captures the focused window when requested |

### Data Storage and External Services

The emulator has no database, cache, or remote service. `TerminalSession` communicates with a local child process through a pseudo-terminal; screenshot requests are handled by the GTK host through the external `scrot` utility.

### Platform Support and Screenshot Dependency

The GTK3 C++ target requires a POSIX host with GTK3, Cairo, and PTY support; its PTY implementation has macOS and other POSIX platform paths. Screenshot capture has narrower support: Tektronix control-W requests launch `scrot --focused`, which requires `scrot` on `PATH` and an X11 session. Screenshot capture is not supported on native macOS or Wayland. If `scrot` is unavailable, the host reports the launch failure and continues without a screenshot.

### Key Architectural Decisions

- GTK is linked privately to the host executable; the core is a separate static library.
- The core issues drawing operations through `TubeRenderer`; the GTK host supplies `CairoTubeRenderer`.
- Cairo is linked privately by the host executable, not by the core library.
- The terminal process and its PTY streams are owned by `TerminalSession`.

## Component Relationships

<!-- mermaid-checked: no \n, no em-dash/en-dash, no {} in labels, subgraphs are id["label"], arrows are -->|"label"|, all subgraphs closed by end, ids unique -->
~~~mermaid
flowchart LR
    subgraph cHost["GTK host callbacks"]
        cMain["main.cpp"]
        cGtkApp["GtkApp in gtk_app.cpp"]
        cCanvas["CairoCanvas"]
        cTimer["Timer callback"]
        cDraw["Draw callback"]
        cKeys["Keyboard callback"]
        cMouse["Mouse callback"]
        cShot["Screenshot launcher"]
    end
    subgraph cCore["Core facade and state"]
        cTube["TubeCore facade"]
        cSession["TerminalSession"]
        cState["TubeCoreState per instance"]
        cContract["TubeRenderer contract"]
    end
    subgraph cProtocol["Protocol interpreters"]
        cTek["tek4010.cpp"]
        cArds["ards.cpp"]
    end
    subgraph cRender["Host rendering"]
        cRenderer["CairoTubeRenderer"]
        cRenderSource["tube_render.cpp"]
        cCairo["Cairo API"]
    end

    cMain -->|"runs"| cGtkApp
    cGtkApp -->|"owns instance"| cTube
    cGtkApp -->|"initializes and calls"| cTube
    cGtkApp -->|"creates and uses"| cCanvas
    cGtkApp -->|"registers"| cTimer
    cGtkApp -->|"registers"| cDraw
    cGtkApp -->|"registers"| cKeys
    cGtkApp -->|"registers"| cMouse
    cTimer -->|"polls and consumes requests"| cTube
    cTimer -->|"launches on request"| cShot
    cDraw -->|"dispatches through"| cTube
    cCanvas -->|"invokes draw callback"| cDraw
    cCanvas -->|"composes surfaces"| cCairo
    cKeys -->|"submits keys or break"| cTube
    cMouse -->|"dispatches click"| cTube
    cTube -->|"Tek mode"| cTek
    cTube -->|"ARDS mode"| cArds
    cTube -->|"owns"| cSession
    cTube -->|"owns instance state"| cState
    cTek -->|"uses input and drawing services"| cTube
    cArds -->|"uses input and drawing services"| cTube
    cTek -->|"requests screenshot"| cTube
    cTek -->|"mutates instance state"| cState
    cArds -->|"mutates instance state"| cState
    cTek -->|"issues drawing operations"| cContract
    cArds -->|"issues drawing operations"| cContract
    cTube -->|"receives per frame"| cContract
    cRenderer -.->|"implements"| cContract
    cRenderSource -->|"implements"| cRenderer
    cDraw -->|"binds Cairo contexts"| cRenderer
    cRenderer -->|"draws primitives"| cCairo
~~~

### Component Inventory

| Component | Layer | Type | Responsibility |
|---|---|---|---|
| `main.cpp` | Host | Entry point | Starts the GTK host |
| `gtk_app.cpp` | Host | GTK application | Owns GTK lifecycle, callbacks, window state, and a `TubeCore` instance |
| `cairo_canvas.cpp` | Host | Cairo backend | Owns offscreen surfaces and viewport scaling, composition, and click mapping |
| `tube.cpp` | Core | Core facade | Owns per-instance core state, session lifecycle, and host-facing operations |
| `terminal_session.cpp` | Core | Process and PTY owner | Starts, polls, communicates with, and closes the terminal child |
| `tek4010.cpp` | Core | Protocol interpreter | Parses Tektronix commands using instance-owned parser state |
| `ards.cpp` | Core | Protocol interpreter | Parses ARDS commands using instance-owned parser state |
| `tube_renderer.h` | Core boundary | Renderer contract | Defines backend-neutral drawing operations passed to `TubeCore` |
| `tube_render.cpp` | Host | Cairo backend | Implements `CairoTubeRenderer` and keeps Cairo contexts out of the core |
| `tube_internal.h` | Core | Internal state and API | Defines `TubeCoreState` and state-aware protocol interfaces |
