add_executable(unit_tests test_suite.cpp)

target_link_libraries(unit_tests PRIVATE api_functions)

enable_testing()
add_test(NAME GeoTests COMMAND unit_tests)