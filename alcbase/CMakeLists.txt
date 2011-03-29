find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})


set(ALCBASE_ALCUTIL_FILES
	alcutil/alccfgtypes.cpp
	alcutil/alclog.cpp
	alcutil/alcos.cpp
	alcutil/alcparser.cpp
	alcutil/alcthread.cpp
	alcutil/md5.cpp
	alcutil/rijndael.cpp
	alcutil/useful.cpp)

set(ALCBASE_URUTYPES_FILES
	urutypes/ageinfo.cpp
	urutypes/plbase.cpp
	urutypes/plmessage.cpp
	urutypes/urubasetypes.cpp
	urutypes/whatdoyousee.cpp)

set(ALCBASE_BASE_FILES
	alcdebug.cpp
	alcmain.cpp
	alctypes.cpp
	alcexception.cpp
	alclicense.cpp
	alcversion.cpp)

add_library(alcbase SHARED ${ALCBASE_ALCUTIL_FILES} ${ALCBASE_BASE_FILES} ${ALCBASE_URUTYPES_FILES})
target_link_libraries(alcbase ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# tests
add_executable(alcbase_test test/alcbase_test.cpp)
target_link_libraries(alcbase_test alcbase)
add_test("AlcbaseTests" alcbase_test "${PROJECT_SOURCE_DIR}")

add_executable(alcthread_test test/alcthread_test.cpp)
target_link_libraries(alcthread_test alcbase)
add_test("AlcthreadTests" alcthread_test)
