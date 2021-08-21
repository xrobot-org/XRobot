# Tool Chain

# Target definition
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain settings
set(CMAKE_C_COMPILER    arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(AR                  arm-none-eabi-ar)
set(OBJCOPY             arm-none-eabi-objcopy)
set(OBJDUMP             arm-none-eabi-objdump)
set(SIZE                arm-none-eabi-size)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# this makes the test compiles use static library option so that we don't need to pre-set linker flags and scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# General Compiler Options

# Options for DEBUG build
# -Og enables optimizations that do not interfere with debugging
# -g produce debugging information in the operating system’s native format
set(CMAKE_C_FLAGS_DEBUG "-Og -g -ggdb3")
set(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

# Options for RELEASE build
# -Os Optimize for size. -Os enables all -O2 optimizations
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
