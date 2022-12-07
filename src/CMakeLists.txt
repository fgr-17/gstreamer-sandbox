cmake_minimum_required(VERSION 3.16)

project(gstreamer_udp_test)

### C++
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard required for build")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "is C++ stndard a requirement")
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "enable/disable compiler specific extensions")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -W -Wall -Wextra")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
message("GSTREAMER_FOUND:" ${GSTREAMER_FOUND})
message("GSTREAMER_LINK_LIBRARIES: " ${GSTREAMER_LINK_LIBRARIES})

add_subdirectory(local)
add_subdirectory(remote)
add_subdirectory(common)
