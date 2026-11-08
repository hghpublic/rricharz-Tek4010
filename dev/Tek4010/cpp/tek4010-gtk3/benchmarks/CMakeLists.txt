
set(target tek4010-gtk3-core-benchmarks)
add_executable(${target}
    core_benchmarks.cpp
    plt_render_benchmarks.cpp
)
set_target_properties(${target} PROPERTIES
    CXX_SCAN_FOR_MODULES ON
    FOLDER "tek4010-cpp-gtk3/benchmarks"
)
add_dependencies(${target} ${tek4010_gtk3_target})
target_compile_definitions(${target} PRIVATE
    TEK4010_GTK3_APP_PATH="$<TARGET_FILE:${tek4010_gtk3_target}>"
    TEK4010_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
target_link_libraries(${target} PRIVATE
    ${tek4010_gtk3_core_target}
    Catch2::Catch2WithMain
)
