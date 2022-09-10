cmake_minimum_required(VERSION 3.11)

set(BSP_DIR ${HW_DIR}/bsp)
set(MCU_DIR ${HW_DIR}/mcu)

if(CONFIG_BOARD_DEV_C)
  set(BOARD_DIR ${BSP_DIR}/rm-c)
elseif(CONFIG_BOARD_WEARLAB_NODE)
  set(BOARD_DIR ${BSP_DIR}/wear_lab_node)
else()
  message(FATAL_ERROR "No board selected.")
endif()

set(CMAKE_MODULE_PATH ${BOARD_DIR})
include(board)
execute_process(COMMAND cp ./debug/launch.json ${CMAKE_CURRENT_SOURCE_DIR}/.vscode WORKING_DIRECTORY ${BOARD_DIR})
