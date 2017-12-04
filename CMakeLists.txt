cmake_minimum_required(VERSION 3.9)
project(bomberman C CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 11)

add_library(common STATIC common/protocol.h)
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)

add_executable(client client/main.cpp)
target_link_libraries(client common)

add_executable(server server/main.c server/setup.c server/setup.h server/player.c server/player.h)
target_link_libraries(server common Threads::Threads)