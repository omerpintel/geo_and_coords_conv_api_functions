cmake_minimum_required(VERSION 3.21)
set(CMAKE_SUPPRESS_REGENERATION ON CACHE BOOL "Suppress ZERO_CHECK auto-regeneration" FORCE)
project(ApiFunctions VERSION 0.1.0 LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(API_FUNCTIONS_GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/include")
configure_file(
    "${CMAKE_SOURCE_DIR}/include/api_version.h.in"
    "${CMAKE_SOURCE_DIR}/include/api_version.h"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/include/api_version.h.in"
    "${API_FUNCTIONS_GENERATED_INCLUDE_DIR}/api_version.h"
    @ONLY
)

if(WIN32)
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/api_functions_version.rc.in"
        "${CMAKE_BINARY_DIR}/generated/api_functions_version.rc"
        @ONLY
    )
endif()

# --- Sanitizer Options ---
option(ENABLE_ASAN "Enable AddressSanitizer for memory error detection" OFF)

if(ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Define a specific header file to be forced into every compilation unit.
# safty rules preventing heap allocation
set(SAFETY_HEADER "${CMAKE_SOURCE_DIR}/include/no_heap.h")
add_library(safety_flags INTERFACE)

# Apply platform-specific compiler flags:
# - MSVC:
#     /FI: Forces the include of the safety header.
#     /W4: Sets warning level to 4 (high).
# - GCC/Clang (else):
#     -include: Forces the include of the safety header.
#     -Wall -Wextra: Enables standard and extra warning sets.
if (MSVC)
    target_compile_options(safety_flags INTERFACE
        "$<$<COMPILE_LANGUAGE:CXX>:/FI${SAFETY_HEADER}>"
        "$<$<COMPILE_LANGUAGE:CXX>:/W4>"
    )
else()
    target_compile_options(safety_flags INTERFACE
        "$<$<COMPILE_LANGUAGE:CXX>:-include${SAFETY_HEADER}>"
        "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
        "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
    )
endif()

add_subdirectory(src)

enable_testing()
add_subdirectory(tests/unit)
