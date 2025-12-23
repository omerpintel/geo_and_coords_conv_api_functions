cmake_minimum_required(VERSION 3.15)
project(ApiFunctions CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
add_subdirectory(tests)