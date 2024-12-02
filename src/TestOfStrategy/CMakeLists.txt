add_executable (testOfStrategy "testOfStrategy.cpp" )
target_sources(testOfStrategy
  PUBLIC
    FILE_SET CXX_MODULES FILES
      "TickParser.cpp" )

target_link_libraries(testOfStrategy "BacktestingLib" "MovingAverageRobot" "Utils")
