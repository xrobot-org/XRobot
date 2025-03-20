cmake_minimum_required(VERSION 3.14)
project(XRobot2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SRC_FILES src/*.cpp)
add_library(xrobot STATIC ${SRC_FILES})

add_executable(example_app examples/app_main.cpp)
target_link_libraries(example_app xrobot)