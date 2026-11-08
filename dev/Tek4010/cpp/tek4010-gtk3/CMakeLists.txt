enable_language(CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(tek4010_gtk3_target tek4010-gtk3-cpp)
set(tek4010_gtk3_core_target tek4010-gtk3-core)
option(TEK4010_BUILD_CATCH2_BENCHMARKS
    "Build Catch2 microbenchmarks for the GTK3 core"
    OFF
)

add_executable(${tek4010_gtk3_target})
set_target_properties(${tek4010_gtk3_target} PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)
add_library(${tek4010_gtk3_core_target} STATIC)
set_target_properties(${tek4010_gtk3_core_target} PROPERTIES
    CXX_SCAN_FOR_MODULES ON
    FOLDER "tek4010-cpp-gtk3/core"
)
target_link_libraries(${tek4010_gtk3_target} PRIVATE
    ${tek4010_gtk3_core_target}
    PkgConfig::GTK3_LEGACY
    PkgConfig::CAIRO_LEGACY
)
target_link_libraries(${tek4010_gtk3_core_target} PRIVATE
    scn::scn
)
set_target_properties(${tek4010_gtk3_target} PROPERTIES FOLDER "tek4010-cpp-gtk3")

if(BUILD_TESTING OR TEK4010_BUILD_CATCH2_BENCHMARKS)
    find_package(Catch2 CONFIG REQUIRED)
endif()

if(BUILD_TESTING)
    include(Catch)

    add_executable(tek4010-gtk3-core-instance-test
        tests/core_instance_test.cpp
    )
    set_target_properties(tek4010-gtk3-core-instance-test PROPERTIES
        CXX_SCAN_FOR_MODULES ON
        FOLDER "tek4010-cpp-gtk3/tests"
    )
    target_link_libraries(tek4010-gtk3-core-instance-test PRIVATE
        ${tek4010_gtk3_core_target}
        Catch2::Catch2WithMain
    )
    catch_discover_tests(tek4010-gtk3-core-instance-test
        TEST_PREFIX "tek4010-gtk3-core-instance:"
    )

    add_executable(tek4010-gtk3-cairo-render-test
        tests/cairo_renderer_test.cpp
        src/cairo_canvas.cpp
        src/tube_render.cpp
    )
    set_target_properties(tek4010-gtk3-cairo-render-test PROPERTIES
        CXX_SCAN_FOR_MODULES ON
        FOLDER "tek4010-cpp-gtk3/tests"
    )
    target_sources(tek4010-gtk3-cairo-render-test
        PRIVATE FILE_SET cairo_test_modules TYPE CXX_MODULES
        FILES
            src/tube_renderer.cppm
            src/cairo_canvas.cppm
            src/cairo_tube_renderer.cppm
    )
    target_link_libraries(tek4010-gtk3-cairo-render-test PRIVATE
        PkgConfig::CAIRO_LEGACY
        Catch2::Catch2WithMain
    )
    catch_discover_tests(tek4010-gtk3-cairo-render-test
        TEST_PREFIX "tek4010-gtk3-cairo-render:"
    )
endif()

if(TEK4010_BUILD_CATCH2_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

set(tek4010_gtk3_demos
    demo
    fdemo
    gdemo
    idemo
)

foreach(demo IN LISTS tek4010_gtk3_demos)
    set(demo_target "${tek4010_gtk3_target}-${demo}")
    add_custom_target(${demo_target}
        COMMAND $<TARGET_FILE:${tek4010_gtk3_target}> demos/${demo}.sh
        DEPENDS ${tek4010_gtk3_target}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
    )
    set_target_properties(${demo_target} PROPERTIES FOLDER "tek4010-cpp-gtk3/demos")
endforeach()

add_subdirectory(src)
