set(tek4010_gtk3_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${tek4010_gtk3_generated_dir}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/help.h.txt"
    "${tek4010_gtk3_generated_dir}/help.h"
    COPYONLY
)

target_sources(${tek4010_gtk3_target} PRIVATE
    main.cpp
    gtk_app.cpp
    cairo_canvas.cpp
    tube_render.cpp
)

target_sources(${tek4010_gtk3_target}
    PRIVATE FILE_SET app_modules TYPE CXX_MODULES
    FILES
        gtk_app.cppm
        cairo_canvas.cppm
        cairo_tube_renderer.cppm
)

target_sources(${tek4010_gtk3_core_target} PRIVATE
    tek4010.cpp
    ards.cpp
    tube.cpp
    tube_render_operations.cpp
    tube_runtime.cpp
    tube_state.cpp
    terminal_session.cpp
    "${tek4010_gtk3_generated_dir}/help.h"
)

target_sources(${tek4010_gtk3_core_target}
    PUBLIC FILE_SET core_modules TYPE CXX_MODULES
    FILES
        terminal_session.cppm
        tube_renderer.cppm
        tube.cppm
        tube_internal.cppm
)

# Keep both parsers in the tube module because they share TubeCoreState.
target_sources(${tek4010_gtk3_core_target}
    PRIVATE FILE_SET parser_partitions TYPE CXX_MODULES
    FILES
        tek4010.cpp
        ards.cpp
)

target_include_directories(${tek4010_gtk3_core_target} PRIVATE
    "${tek4010_gtk3_generated_dir}"
)
