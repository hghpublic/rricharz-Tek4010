find_program(HOMEBREW_GCC_16_C_COMPILER NAMES gcc-16 REQUIRED)
find_program(HOMEBREW_GCC_16_CXX_COMPILER NAMES g++-16 REQUIRED)

set(CMAKE_C_COMPILER "${HOMEBREW_GCC_16_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${HOMEBREW_GCC_16_CXX_COMPILER}")

# Meson packages use the detected shared-linker flags for executable link steps.
# GNU libintl requires CoreFoundation on macOS.
foreach(config_type IN ITEMS DEBUG RELEASE)
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_${config_type}_INIT
        " -Wl,-framework -Wl,CoreFoundation"
    )
endforeach()
