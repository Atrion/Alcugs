add_executable(alcmsgtest alcmsgtest.cpp)
target_link_libraries(alcmsgtest alcnet)

add_executable(alcping alcping.cpp)
target_link_libraries(alcping alcnet)

add_executable(alcugs_lobby alcugs_main.cpp lobbyserver.cpp lobbyserver.h)
target_link_libraries(alcugs_lobby alcnet)

add_executable(alcugs_game alcugs_main.cpp gameserver.cpp gameserver.h sdl.cpp sdl.h sdlbinary.cpp sdlbinary.h)
target_link_libraries(alcugs_game alcnet)

add_executable(alcugs_tracking alcugs_main.cpp trackingserver.cpp trackingserver.h trackingbackend.cpp trackingbackend.h)
target_link_libraries(alcugs_tracking alcnet)

add_executable(alcugs_auth alcugs_main.cpp authserver.cpp authserver.h authbackend.cpp authbackend.h)
target_link_libraries(alcugs_auth alcnet)

add_executable(alcugs_vault alcugs_main.cpp vaultserver.cpp vaultserver.h vaultbackend.cpp vaultbackend.h vaultdb.cpp vaultdb.h)
target_link_libraries(alcugs_vault alcnet)