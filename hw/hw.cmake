cmake_minimum_required(VERSION 3.11)

set(BSP_DIR ${HW_DIR}/bsp)
set(MCU_DIR ${HW_DIR}/mcu)

FILE(GLOB children RELATIVE ${BSP_DIR} ${BSP_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    set(BOARD_DIR ${child})
  ENDIF()
ENDFOREACH()

set(CMAKE_MODULE_PATH ${BSP_DIR}/${BOARD_DIR})
include(board)
execute_process(COMMAND cp ./debug/launch.json ${CMAKE_CURRENT_SOURCE_DIR}/.vscode WORKING_DIRECTORY ${BOARD_DIR})
