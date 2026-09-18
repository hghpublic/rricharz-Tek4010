cmake_minimum_required(VERSION 4.3)

project(tek4010
    VERSION 2.0.2
    LANGUAGES C
    HOMEPAGE_URL "https://github.com/rricharz/Tek4010"
    DESCRIPTION "Free Tektronix 4010, 4013, 4014 and 4015 terminal emulator."
)

# tek4010

# pkg-config
find_package(PkgConfig REQUIRED)

# gtk
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

# cairo
pkg_check_modules(CAIRO REQUIRED cairo)
include_directories(${CAIRO_INCLUDE_DIRS})
link_directories(${CAIRO_LIBRARY_DIRS})
add_definitions(${CAIRO_CFLAGS_OTHER})




set(target tek4010)
add_executable(${target})
add_subdirectory(src)

target_link_libraries(${target} PRIVATE ${CAIRO_LIBRARIES})
target_link_libraries(${target} PRIVATE ${GTK3_LIBRARIES})
