# Architecture option
set(CPU "-mcpu=cortex-m4")
set(ENDIAN "-mlittle-endian")
set(FPU "-mfpu=fpv4-sp-d16")
set(FLOAT_ABI "-mfloat-abi=hard")

set(MCU_FLAGS "${CPU} ${ENDIAN} -mthumb ${FPU} ${FLOAT_ABI}")

# TODO: Remove "-w" & Add "-Wall -Wextra"
set(GENERAL_FLAGS "-fno-builtin -w -ffunction-sections -fdata-sections -mabi=aapcs")

set(CMAKE_C_FLAGS "${MCU_FLAGS} ${GENERAL_FLAGS}")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} ${GENERAL_FLAGS} -x assembler-with-cpp")

# Linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F407IGHx_FLASH.ld)

# Linker Flag
set(LINK_LIB "-lc -lm -lnosys")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} --specs=nosys.specs --specs=nano.specs -mabi=aapcs -T ${LINKER_SCRIPT} ${LINK_LIB} -Wl,-Map=${CMAKE_PROJECT_NAME}.map,--cref,--gc-sections")

# 处理器相关宏定义
add_compile_definitions(
    ARM_MATH_CM4
    STM32F407xx
)
