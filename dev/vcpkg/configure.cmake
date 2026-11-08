if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/triplets" CACHE STRING "vcpkg workspace triplets")
    set(_tek4010_vcpkg_overlay_ports "${CMAKE_CURRENT_LIST_DIR}/ports")
    if(VCPKG_OVERLAY_PORTS)
        list(APPEND _tek4010_vcpkg_overlay_ports ${VCPKG_OVERLAY_PORTS})
    endif()
    list(REMOVE_DUPLICATES _tek4010_vcpkg_overlay_ports)
    set(VCPKG_OVERLAY_PORTS
        "${_tek4010_vcpkg_overlay_ports}"
        CACHE STRING "vcpkg workspace overlay ports"
        FORCE
    )
    unset(_tek4010_vcpkg_overlay_ports)
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_LIST_DIR}/triplets/toolchains/homebrew-gcc-16.cmake"
        CACHE FILEPATH "Homebrew GCC 16 toolchain for vcpkg"
    )

    find_program(_tek4010_homebrew_gcc_16 NAMES gcc-16 REQUIRED)
    execute_process(
        COMMAND "${_tek4010_homebrew_gcc_16}" -dumpmachine
        RESULT_VARIABLE _tek4010_gcc16_result
        OUTPUT_VARIABLE _tek4010_gcc16_target
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT _tek4010_gcc16_result EQUAL 0)
        message(FATAL_ERROR "Unable to determine the target architecture of Homebrew GCC 16")
    elseif(_tek4010_gcc16_target MATCHES "^(arm64|aarch64)")
        set(_tek4010_vcpkg_triplet arm64-osx-gcc-16)
    elseif(_tek4010_gcc16_target MATCHES "^(x86_64|i[3-6]86)")
        set(_tek4010_vcpkg_triplet x64-osx-gcc-16)
    else()
        message(FATAL_ERROR "Unsupported Homebrew GCC 16 target: ${_tek4010_gcc16_target}")
    endif()

    set(VCPKG_TARGET_TRIPLET "${_tek4010_vcpkg_triplet}" CACHE STRING "vcpkg target triplet")
endif()
