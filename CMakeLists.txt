cmake_minimum_required(VERSION 3.9)
project(bomberman C CXX)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
find_package(SFML REQUIRED system window graphics network audio)

include_directories(${SFML_INCLUDE_DIR})

add_executable(client client/main.cpp)
target_link_libraries(client ${SFML_LIBRARIES})

add_executable(server server/main.c server/setup.c server/setup.h server/player.c server/player.h)
target_link_libraries(server Threads::Threads)