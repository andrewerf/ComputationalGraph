cmake_minimum_required(VERSION 3.20)
project(ComputationalGraph)

set(CMAKE_CXX_STANDARD 20)
include_directories(include)

add_executable(ComputationalGraph main.cpp)
target_link_libraries(ComputationalGraph pthread)