cmake_minimum_required(VERSION 3.16)

add_library(Common utils.cpp)
target_include_directories (Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})