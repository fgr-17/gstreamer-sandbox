cmake_minimum_required(VERSION 3.16)


set(target_name gstreamer-common)




add_library(${target_name} utils.cpp gst-utils.cpp)
target_include_directories (${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${target_name} PRIVATE  ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(${target_name} ${GSTREAMER_LINK_LIBRARIES})