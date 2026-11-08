include("${CMAKE_CURRENT_LIST_DIR}/../meson-clang-libstdcxx.cmake")

set(_tek4010_harfbuzz_port_dir "${CURRENT_PORT_DIR}")
set(CURRENT_PORT_DIR "${VCPKG_ROOT_DIR}/ports/harfbuzz")
include("${VCPKG_ROOT_DIR}/ports/harfbuzz/portfile.cmake")
set(CURRENT_PORT_DIR "${_tek4010_harfbuzz_port_dir}")
