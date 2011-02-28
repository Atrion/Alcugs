cmake_minimum_required(VERSION 2.6)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
project(Alcugs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
enable_testing()
find_package (Threads REQUIRED)


# compiler flags
set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra -Wold-style-cast ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")

# options
option(ENABLE_ADMIN "Enables possibly dangerous admin options in alcping (default OFF)" OFF)
option(ENABLE_MSGLOG "Enables detailed logs about incoming and outgoing messages (default ON)" ON)
option(ENABLE_ACKLOG "Enables logs of incoming and outgoing ack messages (default OFF)" OFF)
option(ENABLE_ACKDEBUG "Enables detailed logs about the ack stack (default OFF)" OFF)
option(ENABLE_NETDEBUG "Enables noise and latency debuging tests (default NO)" OFF)
option(ENABLE_MSGDEBUG "Enables (byte-level) logging of message content (default OFF)" OFF)


# alcbase
configure_file(
  "${PROJECT_SOURCE_DIR}/alcbase/alcconfig.h.in"
  "${PROJECT_BINARY_DIR}/alcbase/alcconfig.h"
  )
include_directories("${PROJECT_SOURCE_DIR}/alcbase")
include_directories("${PROJECT_BINARY_DIR}/alcbase") # we need to find the configure file
add_subdirectory(alcbase)

# alcnet
include_directories("${PROJECT_SOURCE_DIR}/alcnet")
add_subdirectory(alcnet)

# servers
add_subdirectory(servers)
