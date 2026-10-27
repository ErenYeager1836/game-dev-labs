cmake_minimum_required(VERSION 3.16)
project(ecs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Header-only interface library for ECS
add_library(ecs_core INTERFACE)
target_include_directories(ecs_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Standalone CLI Demo
add_executable(ecs_demo src/main_ecs.cpp)
target_link_libraries(ecs_demo PRIVATE ecs_core)

# Unit Tests Executable
add_executable(ecs_tests tests/test_ecs.cpp)
target_link_libraries(ecs_tests PRIVATE ecs_core)

add_test(NAME ECSTest COMMAND ecs_tests)
