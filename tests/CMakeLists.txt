add_executable(geo_unit_tests geometric_test.cpp)
add_executable(coords_unit_tests coords_conv_test.cpp)

target_link_libraries(geo_unit_tests PRIVATE api_functions)
target_link_libraries(coords_unit_tests PRIVATE api_functions)

enable_testing()
add_test(NAME GeoTests COMMAND unit_tests)
add_test(NAME CoordsTests COMMAND coords_unit_tests)