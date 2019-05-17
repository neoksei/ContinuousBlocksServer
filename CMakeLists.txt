cmake_minimum_required(VERSION 3.14)
project(ContinuousBlocks)

set(CMAKE_CXX_STANDARD 17)

add_executable(ContinuousBlocks main.cpp FileSystem.cpp FileSystem.h)

find_package(cppzmq)
target_link_libraries(ContinuousBlocks cppzmq)