#set document(title: "GLT3")
#set par(justify: true)
#set page(margin: 40mm)
#set text(font: "Arial", size: 10pt)
#set heading(numbering: "1.1")

#align(center)[
  #text(size: 18pt, weight: "bold")[GLT3]
]

#outline(title: [Contents], depth: 2)

= References

- GTK website: #link("https://www.gtk.org/")[https://www.gtk.org/]
- GTK source repository: #link("https://gitlab.gnome.org/GNOME/gtk/")[https://gitlab.gnome.org/GNOME/gtk/]
- GTK 3 documentation: #link("https://docs.gtk.org/gtk3/")[https://docs.gtk.org/gtk3/]
- GTK 3 CMake examples: #link("https://github.com/shlomif/gtk3-cmake-examples")[https://github.com/shlomif/gtk3-cmake-examples]

= CMake example

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})
# Add an executable compiled from hello.c
add_executable(hello hello.c)
# Link the target to the GTK+ libraries
target_link_libraries(hello ${GTK3_LIBRARIES})
```
