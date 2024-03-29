find_package(MySQL REQUIRED)
include_directories(${MYLIB_INCLUDE_DIR})

require_include("netdb.h")
require_include("sys/socket.h")
require_include("netinet/in.h")

set (ALCNET_PROTOCOL_FILES
	protocol/protocol.cpp
	protocol/umsgbasic.cpp
	protocol/authmsg.cpp
	protocol/gamemsg.cpp
	protocol/lobbybasemsg.cpp
	protocol/lobbymsg.cpp
	protocol/trackingmsg.cpp
	protocol/vaultmsg.cpp
	protocol/vaultproto.cpp)

set (ALCNET_BASE_FILES
	sql.cpp
	unet.cpp
	netsession.cpp
	unetbase.cpp
	unetlobbyserverbase.cpp
	unetmain.cpp
	unetserverbase.cpp
	netsessionmgr.cpp)

add_library(alcnet SHARED ${ALCNET_BASE_FILES} ${ALCNET_PROTOCOL_FILES})
target_link_libraries(alcnet alcbase ${MYSQL_LIBRARY} ${MYSQL_EXTRA_LIBRARIES})

# tests
add_executable(bincomp test/bincomp.cpp)
target_link_libraries(bincomp alcbase)
add_test("NetworkTest" "${PROJECT_SOURCE_DIR}/alcnet/test/alcmsgtest.sh")
