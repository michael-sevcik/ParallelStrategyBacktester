add_subdirectory(test)

add_library(Utils)
target_sources(Utils
  PUBLIC
    FILE_SET CXX_MODULES FILES
      Utils.ixx CSVParser.cpp )


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Utils PROPERTY CXX_STANDARD 20)
endif()
