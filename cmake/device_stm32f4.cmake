#---------------------------------------------------------------------------------------
# Device specific options
set(CPU "-mcpu=cortex-m4")
set(FPU "-mfpu=fpv4-sp-d16")
set(FLOAT_ABI "-mfloat-abi=hard")

set(MCU "${CPU} -mthumb ${FPU} ${FLOAT_ABI}")

set(GENERAL_FLAGS "-fno-builtin -Wall -Wextra -Werror -ffunction-sections -fdata-sections -mabi=aapcs")

set(CMAKE_C_FLAGS "${MCU} ${GENERAL_FLAGS}")
set(CMAKE_ASM_FLAGS "${MCU} {GENERAL_FLAGS} -x assembler-with-cpp")

# Linker flags
set(LINK_LIB "-lc -lm -lnosys")
set(CMAKE_EXE_LINKER_FLAGS "${MCU} --specs=nosys.specs --specs=nano.specs -mabi=aapcs -T ${LINKER_SCRIPT} ${LINK_LIB} -Wl, -Map=${CMAKE_PROJECT_NAME}.map, --cref -Wl, --gc-sections")

add_compile_definitions(ARM_MATH_CM4)
