# Target definition
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR RISCV)

# Toolchain settings
set(CMAKE_C_COMPILER    riscv-none-embed-gcc)
set(CMAKE_CXX_COMPILER  riscv-none-embed-g++)
set(CMAKE_ASM_COMPILER  riscv-none-embed-gcc)
set(CMAKE_LINKER        riscv-none-embed-ld)
set(CMAKE_AR            riscv-none-embed-ar)
set(CMAKE_SIZE          riscv-none-embed-size)
set(CMAKE_OBJCOPY       riscv-none-embed-objcopy)
set(CMAKE_OBJDUMP       riscv-none-embed-objdump)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# this makes the test compiles use static library option so that we don't need to pre-set linker flags and scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
