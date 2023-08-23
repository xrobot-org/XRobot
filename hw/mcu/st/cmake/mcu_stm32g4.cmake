include(${MCU_DIR}/default/custom_printf.cmake)

set(CPU_FLAGS
    "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb"
    CACHE INTERNAL "" FORCE)

set(GENERAL_FLAGS
    "-Wall -Wextra -fno-builtin -fno-exceptions  -ffunction-sections -fdata-sections"
    CACHE INTERNAL "" FORCE)

set(CMAKE_C_FLAGS
    "${CPU_FLAGS} ${GENERAL_FLAGS} -fshort-enums -fdiagnostics-color=auto"
    CACHE INTERNAL "" FORCE)

set(CMAKE_CXX_FLAGS
    "${CPU_FLAGS} ${GENERAL_FLAGS} -fno-threadsafe-statics -fno-rtti -fshort-enums -fdiagnostics-color=auto"
    CACHE INTERNAL "" FORCE)

set(CMAKE_ASM_FLAGS
    "${CPU_FLAGS} -x assembler-with-cpp"
    CACHE INTERNAL "" FORCE)

# Linker Flag
set(LINKER_SCRIPT ${BOARD_DIR}/ld/LinkerScripts.ld)

set(CMAKE_EXE_LINKER_FLAGS
    "-T${LINKER_SCRIPT} --specs=nano.specs --specs=nosys.specs -Wl,--cref,--gc-sections,--print-memory-usage,-Map=${CMAKE_PROJECT_NAME}.map"
    CACHE INTERNAL "" FORCE)

# 处理器相关宏定义
add_compile_definitions(ARM_MATH_CM4)
