cmake_minimum_required(VERSION 4.3)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    # Set the Homebrew GCC 16 vcpkg triplet before project() initializes compilers.
    include("${CMAKE_CURRENT_LIST_DIR}/dev/vcpkg/configure.cmake")
endif()

project(tek4010
    VERSION 2.0.2
    LANGUAGES C
    HOMEPAGE_URL "https://github.com/rricharz/Tek4010"
    DESCRIPTION "Free Tektronix 4010, 4013, 4014 and 4015 terminal emulator."
)

include(CTest)

# tek4010

# pkg-config
find_package(PkgConfig REQUIRED)

# gtk
pkg_check_modules(GTK3_LEGACY REQUIRED IMPORTED_TARGET gtk+-3.0)

# cairo
pkg_check_modules(CAIRO_LEGACY REQUIRED IMPORTED_TARGET cairo)

find_package(scn CONFIG REQUIRED)

set(target tek4010)
add_executable(${target})
add_subdirectory(src)
set_target_properties(${target} PROPERTIES FOLDER "tek4010")

target_link_libraries(${target} PRIVATE
    PkgConfig::CAIRO_LEGACY
    PkgConfig::GTK3_LEGACY
)

# demos

set(demos
    demo
    fdemo
    gdemo
    idemo
)

foreach(demo ${demos})
    add_custom_target(${demo}
        COMMAND $<TARGET_FILE:${target}> demos/${demo}.sh
        DEPENDS ${target}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
    )
    set_target_properties(${demo} PROPERTIES FOLDER "tek4010/demos")
endforeach()

# tektests

add_subdirectory(tektests)

# apl
    
add_subdirectory(apl)

# tek4010-cpp

add_subdirectory(dev/Tek4010)
