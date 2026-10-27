cmake_minimum_required(VERSION 3.16)
project(game_dev_labs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# Subprojects
add_subdirectory(ecs)
add_subdirectory(physics-2d)
add_subdirectory(pathfinding-procgen)
add_subdirectory(game-loop-fsm)
