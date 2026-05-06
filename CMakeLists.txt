cmake_minimum_required(VERSION 3.21)
project(ApiFunctions CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
    target_compile_options(safety_flags INTERFACE "/FI${SAFETY_HEADER}" /W4)
else()
    target_compile_options(safety_flags INTERFACE "-include${SAFETY_HEADER}" -Wall -Wextra)
endif()

add_subdirectory(src)

enable_testing()
add_subdirectory(tests/unit)