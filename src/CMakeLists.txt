cmake_minimum_required(VERSION 3.10)

set(API_FUNCTIONS_SHARED_SOURCES
    "functions.cpp"
    "no_heap.cpp"
    "geometric_functions.cpp"
    "coords_conv_functions.cpp"
    "spherical_functions.cpp"
)

if(WIN32)
    list(APPEND API_FUNCTIONS_SHARED_SOURCES "${CMAKE_BINARY_DIR}/generated/api_functions_version.rc")
endif()

add_library(api_functions SHARED ${API_FUNCTIONS_SHARED_SOURCES})

target_compile_definitions(api_functions PRIVATE API_FUNCTIONS_LIB_EXPORTS)

target_include_directories(api_functions PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/generated/include"
)

target_link_libraries(api_functions PRIVATE safety_flags)

set_target_properties(api_functions PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

# Static library for testing internal (non-exported) functions
add_library(api_functions_static STATIC "functions.cpp" "geometric_functions.cpp" "coords_conv_functions.cpp" "spherical_functions.cpp")

target_compile_definitions(api_functions_static PUBLIC API_FUNCTIONS_LIB_EXPORTS)

target_include_directories(api_functions_static PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/generated/include"
)

target_link_libraries(api_functions_static PRIVATE safety_flags)
