cmake_minimum_required(VERSION 3.16)

set(app_name gstreamer-local)

add_executable (${app_name}
local.cpp)

message("App name: " ${app_name})

target_include_directories(${app_name} PRIVATE  ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(${app_name} ${GSTREAMER_LINK_LIBRARIES})
target_link_libraries(${app_name} Common)
