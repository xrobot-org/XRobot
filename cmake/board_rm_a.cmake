# Board config

# board specific directory for sources and headers
set(BOARD_DIR "rm_a")

# MCU config
include(mcu_stm32f4.cmake)

# Board definitions
add_compile_definitions(
    BOARD_RM_C
)
