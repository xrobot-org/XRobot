cmake_minimum_required(VERSION 3.11)

set(BOARD_NAME Webots)

set(BOARD_DIR ${BSP_DIR}/${BOARD_NAME})

set(WEBOTS_HOME /usr/local/webots)

set(USE_SIMULATOR true)

add_compile_definitions(USE_SIMULATOR)

add_subdirectory(${BOARD_DIR}/drivers)
