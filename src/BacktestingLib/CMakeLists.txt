# CMakeList.txt : CMake project for BacktestingLib, include source and define
# project specific logic here.
#

# Add source to this project's executable.
#add_executable (Backtesting "BacktestingLib.cpp" "BacktestingLib.h" "Backtesting.ixx" "backtester.cpp")
add_library(BacktestingLib)

target_link_libraries(BacktestingLib "AlgoTrading")
target_sources(BacktestingLib
  PUBLIC
    FILE_SET CXX_MODULES FILES
     SimulatedBrokerConnection.cpp  "Backtesting.ixx" "StrategyTester.cpp"  "MarketDataManager.cpp" "TradingManager.cpp" "StrategyOptimizer.cpp")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  #set_property(TARGET Backtesting PROPERTY CXX_STANDARD 20)
  set_property(TARGET BacktestingLib PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory(test)
