set(CMAKE_EXE_LINKER_FLAGS
    "-Wl,--cref,--print-memory-usage,-Map=${CMAKE_PROJECT_NAME}.map"
)

set(GENERAL_FLAGS
    "-fno-exceptions -ffunction-sections -fdata-sections"
    CACHE INTERNAL "" FORCE)

set(CMAKE_C_FLAGS
    " ${GENERAL_FLAGS} -fdiagnostics-color=auto -Wno-extern-c-compat"
    CACHE INTERNAL "" FORCE)

set(CMAKE_CXX_FLAGS
    " ${GENERAL_FLAGS} -fno-threadsafe-statics -fno-rtti -fdiagnostics-color=auto"
    CACHE INTERNAL "" FORCE)

set(CMAKE_ASM_FLAGS
    "-x assembler-with-cpp"
    CACHE INTERNAL "" FORCE)

add_link_options(-fstack-usage)

include($ENV{IDF_PATH}/tools/cmake/toolchain-esp32c3.cmake)
