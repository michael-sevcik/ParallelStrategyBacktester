add_library(MovingAverageRobot)

target_link_libraries(MovingAverageRobot "AlgoTrading")
target_sources(MovingAverageRobot
  PUBLIC
    FILE_SET CXX_MODULES FILES
      MovingAverageRobot.ixx )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MovingAverageRobot PROPERTY CXX_STANDARD 20)
endif()
