find_program(_tek4010_gcc16_c NAMES gcc-16 REQUIRED)
find_program(_tek4010_gcc16_cxx NAMES g++-16 REQUIRED)

execute_process(
    COMMAND "${_tek4010_gcc16_c}" -print-file-name=libatomic.dylib
    RESULT_VARIABLE _tek4010_atomic_result
    OUTPUT_VARIABLE _tek4010_atomic_library
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${_tek4010_gcc16_cxx}" -print-file-name=libstdc++.dylib
    RESULT_VARIABLE _tek4010_stdlib_result
    OUTPUT_VARIABLE _tek4010_stdlib_library
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "${_tek4010_gcc16_cxx}" -v -E -x c++ /dev/null
    RESULT_VARIABLE _tek4010_include_result
    OUTPUT_QUIET
    ERROR_VARIABLE _tek4010_compiler_info
)

if(NOT _tek4010_atomic_result EQUAL 0 OR NOT EXISTS "${_tek4010_atomic_library}")
    message(FATAL_ERROR "Unable to locate Homebrew GCC 16 libatomic")
endif()
if(NOT _tek4010_stdlib_result EQUAL 0 OR NOT EXISTS "${_tek4010_stdlib_library}")
    message(FATAL_ERROR "Unable to locate Homebrew GCC 16 libstdc++")
endif()
if(NOT _tek4010_include_result EQUAL 0)
    message(FATAL_ERROR "Unable to query Homebrew GCC 16 C++ include paths")
endif()

string(REPLACE "\n" ";" _tek4010_compiler_info_lines "${_tek4010_compiler_info}")
set(_tek4010_collect_includes FALSE)
set(_tek4010_gcc16_cxx_includes "")
foreach(_tek4010_line IN LISTS _tek4010_compiler_info_lines)
    string(STRIP "${_tek4010_line}" _tek4010_line)
    if(_tek4010_line STREQUAL "#include <...> search starts here:")
        set(_tek4010_collect_includes TRUE)
    elseif(_tek4010_line STREQUAL "End of search list.")
        set(_tek4010_collect_includes FALSE)
    elseif(_tek4010_collect_includes AND _tek4010_line MATCHES "/include/c\\+\\+/16($|/)")
        list(APPEND _tek4010_gcc16_cxx_includes "${_tek4010_line}")
    endif()
endforeach()

list(LENGTH _tek4010_gcc16_cxx_includes _tek4010_cxx_include_count)
if(_tek4010_cxx_include_count LESS 3)
    message(FATAL_ERROR "Unable to locate Homebrew GCC 16 C++ standard library headers")
endif()

get_filename_component(_tek4010_atomic_dir "${_tek4010_atomic_library}" DIRECTORY)
get_filename_component(_tek4010_stdlib_dir "${_tek4010_stdlib_library}" DIRECTORY)
set(_tek4010_library_paths "${_tek4010_atomic_dir}" "${_tek4010_stdlib_dir}")
if(DEFINED ENV{LIBRARY_PATH} AND NOT "$ENV{LIBRARY_PATH}" STREQUAL "")
    list(APPEND _tek4010_library_paths "$ENV{LIBRARY_PATH}")
endif()
list(REMOVE_DUPLICATES _tek4010_library_paths)
string(JOIN ":" _tek4010_library_path ${_tek4010_library_paths})
set(ENV{LIBRARY_PATH} "${_tek4010_library_path}")

set(_tek4010_meson_cross_file "${CURRENT_BUILDTREES_DIR}/tek4010-clang-libstdcxx.ini")
set(_tek4010_meson_cross_content
    "[binaries]\n"
    "c = ['/usr/bin/clang']\n"
    "cpp = ['/usr/bin/clang++', '-stdlib=libstdc++', '-L', '${_tek4010_stdlib_dir}'"
)
foreach(_tek4010_include IN LISTS _tek4010_gcc16_cxx_includes)
    string(APPEND _tek4010_meson_cross_content ", '-isystem', '${_tek4010_include}'")
endforeach()
string(APPEND _tek4010_meson_cross_content "]\n")
file(WRITE "${_tek4010_meson_cross_file}" "${_tek4010_meson_cross_content}")
set(VCPKG_MESON_CROSS_FILE "${_tek4010_meson_cross_file}")
