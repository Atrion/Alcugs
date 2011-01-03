add_executable(alcmsgtest alcmsgtest.cpp)
target_link_libraries(alcmsgtest alcnet)

add_executable(alcping alcping.cpp)
target_link_libraries(alcping alcnet)

add_executable(alcugs_lobby alcugs_main.cpp lobbyserver.cpp)
target_link_libraries(alcugs_lobby alcnet)

add_executable(alcugs_game alcugs_main.cpp gameserver.cpp sdl.cpp sdlbinary.cpp)
target_link_libraries(alcugs_game alcnet)

add_executable(alcugs_tracking alcugs_main.cpp trackingserver.cpp trackingbackend.cpp)
target_link_libraries(alcugs_tracking alcnet)

add_executable(alcugs_auth alcugs_main.cpp authserver.cpp authbackend.cpp)
target_link_libraries(alcugs_auth alcnet)

add_executable(alcugs_vault alcugs_main.cpp vaultserver.cpp vaultbackend.cpp vaultdb.cpp)
target_link_libraries(alcugs_vault alcnet)