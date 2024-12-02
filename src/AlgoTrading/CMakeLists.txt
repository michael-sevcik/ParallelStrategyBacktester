set(CMAKE_CXX_SCAN_FOR_MODULES ON) 

add_library(AlgoTrading)
target_sources(AlgoTrading
  PUBLIC
    FILE_SET CXX_MODULES FILES
      AlgoTrading.ixx "MarketData.ixx" "BrokerConnection.ixx" "Robot.cpp" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AlgoTrading PROPERTY CXX_STANDARD 20)
endif()
