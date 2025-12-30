add_executable(geo_unit_tests geometric_test.cpp)

target_link_libraries(geo_unit_tests PRIVATE api_functions)

enable_testing()
add_test(NAME GeoTests COMMAND unit_tests)