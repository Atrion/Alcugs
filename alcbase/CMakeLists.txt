find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

include(CheckIncludeFile)
check_include_file("execinfo.h" HAVE_EXECINFO_H)
check_include_file("pthread.h" HAVE_PTHREAD_H)


set(ALCBASE_ALCUTIL_FILES
	alcutil/alccfgtypes.cpp
	alcutil/alccfgtypes.h
	alcutil/alclog.cpp
	alcutil/alclog.h
	alcutil/alcos.cpp
	alcutil/alcos.h
	alcutil/alcparser.cpp
	alcutil/alcparser.h
	alcutil/alcthread.cpp
	alcutil/alcthread.h
	alcutil/conv_funs.cpp
	alcutil/conv_funs.h
	alcutil/md5.cpp
	alcutil/md5.h
	alcutil/rijndael.cpp
	alcutil/rijndael.h
	alcutil/useful.cpp
	alcutil/useful.h
	alcutil/windoze.cpp
	alcutil/windoze.h)

set(ALCBASE_URUTYPES_FILES
	urutypes/ageinfo.cpp
	urutypes/ageinfo.h
	urutypes/plbase.cpp
	urutypes/plbase.h
	urutypes/plmessage.cpp
	urutypes/plmessage.h
	urutypes/urubasetypes.cpp
	urutypes/urubasetypes.h
	urutypes/uruconsts.h
	urutypes/whatdoyousee.cpp
	urutypes/whatdoyousee.h)

set(ALCBASE_BASE_FILES
	alcdebug.cpp
	alcdebug.h
	alclicense.h
	alcmain.cpp
	alcmain.h
	alctypes.cpp
	alctypes.h
	alcexception.cpp
	alcexception.h
	alclicense.cpp
	alcugs.h
	alcversion.cpp
	alcversion.h
	alcxversion.h)

add_library(alcbase SHARED ${ALCBASE_ALCUTIL_FILES} ${ALCBASE_BASE_FILES} ${ALCBASE_URUTYPES_FILES})
target_link_libraries(alcbase ${ZLIB_LIBRARIES})

# tests
add_executable(alcbase_test test/alcbase_test.cpp)
target_link_libraries(alcbase_test alcbase)
add_test("AlcbaseTests" alcbase_test "${PROJECT_SOURCE_DIR}")

add_executable(alcthread_test test/alcthread_test.cpp)
target_link_libraries(alcthread_test alcbase)
add_test("AlcthreadTests" alcthread_test)
