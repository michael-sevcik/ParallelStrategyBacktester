
add_executable(utilsTests "tests.cpp" "run_all.cpp" "TimePointFromString.cpp"  "CSVParserTests.cpp")

target_link_libraries(utilsTests "gtest" "Utils")

# Let GTest discover the tests
gtest_discover_tests(utilsTests)