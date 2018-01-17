cmake_minimum_required(VERSION 3.1)
project(bomberman C CXX)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
find_package(SFML REQUIRED system window graphics network audio)

include_directories(${SFML_INCLUDE_DIR})

set(COMMON_FOLDER src/common)
set(COMMON ${COMMON_FOLDER}/include/protocol.h ${COMMON_FOLDER}/reader.c ${COMMON_FOLDER}/include/reader.h ${COMMON_FOLDER}/include/utils.h)


set(CLIENT_FOLDER src/client)
set(CLIENT_INCLUDE ${CLIENT_FOLDER}/include/packet.h ${CLIENT_FOLDER}/include/state.h ${CLIENT_FOLDER}/include/draw.hpp)
set(CLIENT_SRC ${CLIENT_FOLDER}/main.cpp ${CLIENT_FOLDER}/packet.c ${CLIENT_FOLDER}/state.c ${CLIENT_FOLDER}/draw.cpp)

add_executable(client ${COMMON} ${CLIENT_INCLUDE} ${CLIENT_SRC})

target_include_directories(client PUBLIC ${CLIENT_FOLDER}/include ${COMMON_FOLDER}/include)
target_link_libraries(client Threads::Threads ${SFML_LIBRARIES})


set(SERVER_FOLDER src/server)
set(SERVER_INCLUDE ${SERVER_FOLDER}/include/game.h ${SERVER_FOLDER}/include/object.h ${SERVER_FOLDER}/include/packet.h ${SERVER_FOLDER}/include/player.h ${SERVER_FOLDER}/include/setup.h)
set(SERVER_SRC ${SERVER_FOLDER}/main.c ${SERVER_FOLDER}/game.c ${SERVER_FOLDER}/object.c ${SERVER_FOLDER}/packet.c ${SERVER_FOLDER}/player.c ${SERVER_FOLDER}/setup.c)

add_executable(server ${COMMON} ${SERVER_INCLUDE} ${SERVER_SRC})

target_include_directories(server PUBLIC ${SERVER_FOLDER}/include ${COMMON_FOLDER}/include)
target_link_libraries(server Threads::Threads m)
