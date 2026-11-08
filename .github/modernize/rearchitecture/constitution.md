# Tek4010 C++26 Port Constitution

1. Keep the existing C application under `src/` unchanged; it is the behavioral reference and remains buildable.
2. Implement the full GTK3 application port only under `dev/Tek4010/cpp/tek4010-gtk3/`.
3. Use C++26 for the new target and preserve GTK3, Cairo, POSIX PTY, CLI, terminal-protocol, and user-visible behavior unless the user approves a change.
4. Preserve supported input streams and demo scripts. Avoid unrelated redesign or feature additions.
5. Validate each implementation increment with the narrowest applicable build and tests. The GUI demo smoke test requires a usable display.
6. Record implementation traceability, direct code-quality review, and remaining validation limits.
