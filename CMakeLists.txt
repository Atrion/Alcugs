cmake_minimum_required (VERSION 2.6)
project (Alcugs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
enable_testing()


# compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wold-style-cast ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-Werror -DENABLE_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}")


# alcbase
configure_file (
  "${PROJECT_SOURCE_DIR}/alcbase/alcconfig.h.in"
  "${PROJECT_BINARY_DIR}/alcbase/alcconfig.h"
  )
include_directories ("${PROJECT_SOURCE_DIR}/alcbase")
include_directories ("${PROJECT_BINARY_DIR}/alcbase") # we need to find the configure file
add_subdirectory (alcbase)

include_directories ("${PROJECT_SOURCE_DIR}/alcnet")
add_subdirectory (alcnet)

add_subdirectory (servers)
