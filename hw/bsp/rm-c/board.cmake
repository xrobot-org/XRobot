cmake_minimum_required(VERSION 3.11)

set(BOARD_NAME rm-c)

set(UART_AI True)
set(SPI_BMI088 True)
set(PWM_BUZZER True)
set(BSP_CAN True)
set(UART_DR16 True)
set(PWM_LASER True)
set(PWM_LED True)
set(UART_REFEREE True)
set(PWM_SERVO True)
set(USB_CDC True)

add_compile_definitions(BOARD_RM_C STM32F407xx HSE_VALUE=12000000
    HSE_STARTUP_TIMEOUT=100)

set(BOARD_DIR ${BSP_DIR}/${BOARD_NAME})
set(HAL_DIR ${MCU_DIR}/st/stm32f4xx_hal_driver)
set(STM32_CMSIS_DIR ${MCU_DIR}/st/cmsis_device_f4)
set(ARM_CMSIS_DIR ${LIB_DIR}/cmsis_5)

include(${MCU_DIR}/st/cmake/mcu_stm32f4.cmake)
include(${MCU_DIR}/st/cmake/toolchain_utils.cmake)

add_subdirectory(${BOARD_DIR}/drivers)
