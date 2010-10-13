find_package(MySQL REQUIRED)
include_directories(${MYLIB_INCLUDE_DIR})

set (ALCNET_PROTOCOL_FILES
	protocol/authmsg.cpp
	protocol/authmsg.h
	protocol/ext-protocol.h
	protocol/gamemsg.cpp
	protocol/gamemsg.h
	protocol/lobbybasemsg.cpp
	protocol/lobbybasemsg.h
	protocol/lobbymsg.cpp
	protocol/lobbymsg.h
	protocol/protocol.cpp
	protocol/protocol.h
	protocol/trackingmsg.cpp
	protocol/trackingmsg.h
	protocol/umsgbasic.cpp
	protocol/umsgbasic.h
	protocol/vaultmsg.cpp
	protocol/vaultmsg.h
	protocol/vaultproto.cpp
	protocol/vaultproto.h)

set (ALCNET_BASE_FILES
	alcnet.h
	sql.cpp
	sql.h
	protocol
	unet.cpp
	unet.h
	unetbase.cpp
	unetbase.h
	unetlobbyserverbase.cpp
	unetlobbyserverbase.h
	unetmain.cpp
	unetmain.h
	unetserverbase.cpp
	unetserverbase.h
	netlog.cpp
	netlog.h
	netmsgq.h
	netsession.cpp
	netsession.h
	netsessionmgr.cpp
	netsessionmgr.h)

add_library(alcnet SHARED ${ALCNET_BASE_FILES} ${ALCNET_PROTOCOL_FILES})
target_link_libraries(alcnet alcbase ${MYSQL_LIBRARY} ${MYSQL_EXTRA_LIBRARIES})

# tests
add_executable(bincomp test/bincomp.cpp)
target_link_libraries(bincomp alcbase)
add_test("NetworkTest" "${PROJECT_SOURCE_DIR}/alcnet/test/alcmsgtest.sh" "${PROJECT_SOURCE_DIR}")
