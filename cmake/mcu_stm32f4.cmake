set(MCU_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# TODO: Remove "-w" & Add "-Wall -Wextra"
set(GENERAL_FLAGS "-w --specs=nano.specs --specs=nosys.specs -fno-builtin -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS "${MCU_FLAGS} ${GENERAL_FLAGS} -fshort-enums -ffast-math -fdiagnostics-color=auto")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -g -x assembler-with-cpp")

# Linker Flag
set(CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} -Wl,--cref,--gc-sections,--print-memory-usage,-Map=${CMAKE_PROJECT_NAME}.map")

# 处理器相关宏定义
add_compile_definitions(
    ARM_MATH_CM4
    STM32F407xx
)
