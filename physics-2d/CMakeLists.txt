cmake_minimum_required(VERSION 3.16)
project(physics_2d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(physics2d_core INTERFACE)
target_include_directories(physics2d_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Standalone CLI Demo
add_executable(physics_demo src/main_physics.cpp)
target_link_libraries(physics_demo PRIVATE physics2d_core)

# Unit Tests Executable
add_executable(physics_tests tests/test_physics.cpp)
target_link_libraries(physics_tests PRIVATE physics2d_core)

add_test(NAME PhysicsTest COMMAND physics_tests)
