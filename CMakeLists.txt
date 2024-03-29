cmake_minimum_required(VERSION 2.6)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
project(Alcugs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
enable_testing()
find_package (Threads REQUIRED)

# include file checking
include(CheckIncludeFile)
function(require_include FILE)
	check_include_file(${FILE} "HAVE_${FILE}")
	if(NOT "HAVE_${FILE}")
		message(FATAL_ERROR "Required header file ${FILE} not found!")
	endif()
endfunction(require_include)

# compiler flags
set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra -Wold-style-cast ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-Werror ${CMAKE_CXX_FLAGS_DEBUG}")

# options
option(ENABLE_MSGLOG "Enables detailed logs about incoming and outgoing messages (default ON)" ON)
option(ENABLE_NETDEBUG "Enables noise and latency debuging tests (default OFF)" OFF)
option(ENABLE_ACKCACHE "Enables caching and collecting acks (default ON)" ON)


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
