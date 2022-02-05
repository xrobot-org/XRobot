set(COMPILER_PATH /usr/bin)
# Target definition
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain settings
set(CMAKE_C_COMPILER    ${COMPILER_PATH}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER  ${COMPILER_PATH}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  ${COMPILER_PATH}/arm-none-eabi-gcc)
set(CMAKE_LINKER        ${COMPILER_PATH}/arm-none-eabi-ld)
set(CMAKE_AR            ${COMPILER_PATH}/arm-none-eabi-ar)
set(CMAKE_SIZE          ${COMPILER_PATH}/arm-none-eabi-size)
set(CMAKE_OBJCOPY       ${COMPILER_PATH}/arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP       ${COMPILER_PATH}/arm-none-eabi-objdump)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# this makes the test compiles use static library option so that we don't need to pre-set linker flags and scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
