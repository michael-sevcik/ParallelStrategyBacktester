add_executable(BacktestingLibTests "MarketDataManagerTests.cpp" "RunTestscpp.cpp")

target_link_libraries(BacktestingLibTests "gtest" "BacktestingLib")

# Let GTest discover the tests
gtest_discover_tests(BacktestingLibTests)