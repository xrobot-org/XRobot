cmake_minimum_required(VERSION 3.14)
project(XRobot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SRC_FILES src/*.hpp)
add_library(XRobot STATIC ${SRC_FILES})

if(NOT TARGET xr)
    add_subdirectory(libxr)
endif()

target_link_libraries(XRobot xr)

target_include_directories(XRobot PRIVATE
    PUBLIC $<TARGET_PROPERTY:xr,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(XROBOT_TEST_BUILD)
    add_executable(example_app examples/app_main.cpp)
    target_link_libraries(example_app XRobot)
endif()
