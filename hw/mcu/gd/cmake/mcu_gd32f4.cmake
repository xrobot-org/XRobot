include(${MCU_DIR}/default/custom_printf.cmake)

set(CPU_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

set(GENERAL_FLAGS
    "-Wall -Wextra -Wstack-usage=1024 --specs=nano.specs --specs=nosys.specs -fno-builtin -ffunction-sections -fdata-sections"
)

set(CMAKE_C_FLAGS
    "${CPU_FLAGS} ${GENERAL_FLAGS} -fshort-enums -ffast-math -fdiagnostics-color=auto"
)
set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")

# Linker Flag
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/ld/${BOARD_DIR}.ld)

set(CMAKE_EXE_LINKER_FLAGS
    "-T${LINKER_SCRIPT} -Wl,--cref,--gc-sections,--print-memory-usage,-Map=${CMAKE_PROJECT_NAME}.map"
)

# 处理器相关宏定义
add_compile_definitions(ARM_MATH_CM4)
