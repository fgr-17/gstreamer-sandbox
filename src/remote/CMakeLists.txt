cmake_minimum_required(VERSION 3.16)

set(app_name gstreamer-remote)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable (${app_name}
remote.cpp)

message("App name: " ${app_name})

target_include_directories(${app_name} PRIVATE  ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(${app_name} ${GSTREAMER_LINK_LIBRARIES})
target_link_libraries(${app_name} gstreamer-common)
target_link_libraries(${app_name} gstapp-1.0)
