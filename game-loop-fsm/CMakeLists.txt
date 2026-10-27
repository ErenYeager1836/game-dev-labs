cmake_minimum_required(VERSION 3.16)
project(game_loop_fsm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(gameloop_core INTERFACE)
target_include_directories(gameloop_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Standalone CLI Demo
add_executable(gameloop_demo src/main_loop.cpp)
target_link_libraries(gameloop_demo PRIVATE gameloop_core)

# Unit Tests Executable
add_executable(gameloop_tests tests/test_game_loop.cpp)
target_link_libraries(gameloop_tests PRIVATE gameloop_core)

add_test(NAME GameLoopTest COMMAND gameloop_tests)
