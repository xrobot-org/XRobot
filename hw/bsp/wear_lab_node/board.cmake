cmake_minimum_required(VERSION 3.11)

set(BOARD_NAME wear_lab_node)

add_compile_definitions(BOARD_WEARLAB_NODE STM32F103xB)

set(BOARD_DIR ${BSP_DIR}/${BOARD_NAME})
set(HAL_DIR ${MCU_DIR}/st/stm32f1xx_hal_driver)
set(STM32_CMSIS_DIR ${MCU_DIR}/st/cmsis_device_f1)
set(ARM_CMSIS_DIR ${LIB_DIR}/cmsis_5)

include(${MCU_DIR}/st/cmake/mcu_stm32f1.cmake)
include(${MCU_DIR}/st/cmake/toolchain_utils.cmake)

add_subdirectory(${BOARD_DIR}/drivers)
