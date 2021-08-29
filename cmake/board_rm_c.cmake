# Board config 

# board specific directory for sources and headers
set(BOARD_DIR "rm_c")

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F407IGHx_FLASH.ld)

# MCU config
include(mcu_stm32f4)

# Board definitions 
add_compile_definitions(
    BOARD_RM_C
)