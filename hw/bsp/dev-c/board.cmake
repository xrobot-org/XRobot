cmake_minimum_required(VERSION 3.11)

add_compile_definitions(
    BOARD_RM_C
    STM32F407xx
    HSE_VALUE=12000000
    HSE_STARTUP_TIMEOUT=100
)

set(BOARD_DIR ${BSP_DIR}/dev-c)
set(HAL_DIR ${MCU_DIR}/st/stm32f4xx_hal_driver)
set(STM32_CMSIS_DIR ${MCU_DIR}/st/cmsis_device_f4)
set(ARM_CMSIS_DIR ${LIB_DIR}/cmsis_5)

set(CMAKE_MODULE_PATH
    ${MCU_DIR}/st/cmake
    ${MCU_DIR}/arm/cmake
)

include(mcu_stm32f4)
include(toolchain_utils)

add_subdirectory(${BSP_DIR}/dev-c/drivers)
