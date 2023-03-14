cmake_minimum_required(VERSION 3.10)
project(common VERSION 2.03)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
add_library(common STATIC mca_utils.c)
add_executable(commont commont.cpp)
target_link_libraries(commont common)


