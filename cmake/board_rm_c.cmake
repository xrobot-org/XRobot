# Board config

# board specific directory for sources and headers
set(BOARD_DIR "rm_c")

# MCU config
include(mcu_stm32f4)

# Board definitions
add_compile_definitions(
    BOARD_RM_C
    STM32F407xx
    HSE_VALUE=12000000
    HSE_STARTUP_TIMEOUT=100
)
