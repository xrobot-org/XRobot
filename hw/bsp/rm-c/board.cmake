cmake_minimum_required(VERSION 3.11)

set(BOARD_NAME rm-c)

add_compile_definitions(BOARD_RM_C STM32F407xx HSE_VALUE=12000000
    HSE_STARTUP_TIMEOUT=100)

set(BOARD_DIR ${BSP_DIR}/${BOARD_NAME})
set(HAL_DIR ${MCU_DIR}/st/stm32f4xx_hal_driver)
set(STM32_CMSIS_DIR ${MCU_DIR}/st/cmsis_device_f4)
set(ARM_CMSIS_DIR ${LIB_DIR}/cmsis_5)

include(${MCU_DIR}/st/cmake/mcu_stm32f4.cmake)
include(${MCU_DIR}/st/cmake/toolchain_utils.cmake)

add_subdirectory(${BOARD_DIR}/drivers)
