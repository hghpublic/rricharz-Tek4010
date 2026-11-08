include("${CMAKE_CURRENT_LIST_DIR}/../meson-clang-libstdcxx.cmake")

set(_tek4010_cairo_port_dir "${CURRENT_PORT_DIR}")
set(CURRENT_PORT_DIR "${VCPKG_ROOT_DIR}/ports/cairo")
include("${VCPKG_ROOT_DIR}/ports/cairo/portfile.cmake")
set(CURRENT_PORT_DIR "${_tek4010_cairo_port_dir}")
