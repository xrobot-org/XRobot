include(${MCU_DIR}/default/custom_printf.cmake)

set(CPU_FLAGS
    "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard"
    CACHE INTERNAL "" FORCE)

set(GENERAL_FLAGS
    "-Wall -fdata-sections -ffunction-sections"
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
