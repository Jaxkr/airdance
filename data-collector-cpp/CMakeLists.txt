cmake_minimum_required(VERSION 3.16.3)
project(hello)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})


set(CMAKE_BUILD_TYPE Debug)
include_directories(include)
file(GLOB SOURCES "src/*.cpp")
add_executable(ad ${SOURCES})
include(cmake/CPM.cmake)
CPMAddPackage(NAME;JoyShockLibrary;GITHUB_REPOSITORY;JibbSmart/JoyShockLibrary;VERSION;2.1.0)
target_link_libraries(ad JoyShockLibrary)
target_link_libraries(ad ${SDL2_LIBRARIES})
