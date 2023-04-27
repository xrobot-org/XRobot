cmake_minimum_required(VERSION 3.11)

set(BSP_DIR ${HW_DIR}/bsp)
set(MCU_DIR ${HW_DIR}/mcu)

FILE(GLOB children RELATIVE ${BSP_DIR} ${BSP_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    set(BOARD_DIR ${BSP_DIR}/${child})
    add_compile_definitions(XROBOT_BOARD=${child})
    set(CMAKE_MODULE_PATH ${BOARD_DIR})
  ENDIF()
ENDFOREACH()

FILE(GLOB children RELATIVE ${USER_DIR}/bsp ${USER_DIR}/bsp/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    set(BOARD_DIR ${USER_DIR}/bsp/${child})
    set(CMAKE_MODULE_PATH ${BOARD_DIR})
    add_compile_definitions(XROBOT_BOARD=${child})
  ELSE()
  ENDIF()
ENDFOREACH()

include(board)
execute_process(COMMAND cp ./debug/launch.json ${CMAKE_CURRENT_SOURCE_DIR}/.vscode WORKING_DIRECTORY ${BOARD_DIR})
