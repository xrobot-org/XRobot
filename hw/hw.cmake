cmake_minimum_required(VERSION 3.11)

set(BSP_DIR ${HW_DIR}/bsp)
set(MCU_DIR ${HW_DIR}/mcu)

set(CMAKE_MODULE_PATH ${BSP_DIR}/dev-c)
include(board)
