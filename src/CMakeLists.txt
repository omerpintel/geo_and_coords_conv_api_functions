cmake_minimum_required(VERSION 3.10)

add_library(api_functions SHARED "functions.cpp"  "no_heap.cpp" "geometric_functions.cpp" "coords_conv_functions.cpp")

target_compile_definitions(api_functions PRIVATE API_FUNCTIONS_LIB_EXPORTS)

target_include_directories(api_functions PUBLIC "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(api_functions PRIVATE safety_flags)

# Static library for testing internal (non-exported) functions
add_library(api_functions_static STATIC "functions.cpp" "geometric_functions.cpp" "coords_conv_functions.cpp")

target_compile_definitions(api_functions_static PUBLIC API_FUNCTIONS_LIB_EXPORTS)

target_include_directories(api_functions_static PUBLIC "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(api_functions_static PRIVATE safety_flags)