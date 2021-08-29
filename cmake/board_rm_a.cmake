# Board config 

# board specific directory for sources and headers
set(BOARD_DIR "rm_a")

# Linker script
# TODO: set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F407IGHx_FLASH.ld)

# MCU config
include(mcu_stm32f4.cmake)

# Board definitions 
add_compile_definitions(
    BOARD_RM_C
)