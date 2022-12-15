cmake_minimum_required(VERSION 3.11)

set(BOARD_NAME MiniPC)

set(BOARD_DIR ${BSP_DIR}/${BOARD_NAME})

add_subdirectory(${BOARD_DIR}/drivers)
