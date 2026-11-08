# Existing Tech Stack

- Language: C99 in `src/`; the root CMake project declares C. The C++ GTK3 destination is currently empty.
- GUI: GTK3 and GDK, with GTK callbacks registered through GObject signals.
- Drawing: Cairo, using persistent and temporary surfaces for phosphor/fading behavior.
- Process and terminal I/O: POSIX pseudo-terminal APIs (`forkpty`), `fork`/`exec`, `waitpid`, `select`, termios, and stdio streams.
- Platform-specific headers: macOS `<util.h>` and non-Apple `<pty.h>`.
- Supporting system tools: `fc-list` for the APL font check; `scrot` for the screenshot escape sequence; `pkill rs232-console` during shutdown.
- Build systems: CMake and Make. The Makefile requests `-std=c99`; CMake resolves GTK3 and Cairo through pkg-config.
- Dependencies: GTK3 and Cairo. CMake's manifest installation currently provides GTK3/Cairo and their transitive dependencies through vcpkg.
- Runtime behavior depends on an available graphical display, a usable PTY implementation, and POSIX process APIs.

No database, ORM, network service, or persisted application data model was found in the analyzed source.
