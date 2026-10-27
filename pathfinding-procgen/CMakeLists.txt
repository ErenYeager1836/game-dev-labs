cmake_minimum_required(VERSION 3.16)
project(pathfinding_procgen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(pathfinding_core INTERFACE)
target_include_directories(pathfinding_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Standalone CLI Demo
add_executable(pathfinding_demo src/main_pathfinding.cpp)
target_link_libraries(pathfinding_demo PRIVATE pathfinding_core)

# Unit Tests Executable
add_executable(pathfinding_tests tests/test_pathfinding.cpp)
target_link_libraries(pathfinding_tests PRIVATE pathfinding_core)

add_test(NAME PathfindingTest COMMAND pathfinding_tests)
