cmake_minimum_required(VERSION 3.10)

add_library(api_functions SHARED api_functions.cpp  "no_heap.cpp")

target_compile_definitions(api_functions PRIVATE API_FUNCTIONS_LIB_EXPORTS)

target_include_directories(api_functions PUBLIC "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(api_functions PRIVATE safety_flags)