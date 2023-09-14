# Target definition
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain settings
set(CMAKE_LINKER arm-none-eabi-ld)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# this makes the test compiles use static library option so that we don't need
# to pre-set linker flags and scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CLANG_TARGET arm-none-eabi)
set(GNU_COMPILER arm-none-eabi)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

execute_process(
    COMMAND arm-none-eabi-gcc -print-sysroot
    OUTPUT_VARIABLE GCC_ARM_NONE_EABI_ROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT EXISTS ${GCC_ARM_NONE_EABI_ROOT})
    if(NOT EXISTS /usr/lib/arm-none-eabi OR NOT EXISTS /usr/include/newlib)
    message(FATAL_ERROR "Could not find arm-none-eabi toolchain.")
    endif()
    file(GLOB GCC_ARM_NONE_EABI_INCLUDE
    "/usr/include/newlib/c++/*/cstddef")

    get_filename_component(GCC_ARM_NONE_EABI_INCLUDE
    "${GCC_ARM_NONE_EABI_INCLUDE}" DIRECTORY)
    add_compile_options(
        --sysroot=/usr/lib/arm-none-eabi
        -isystem${GCC_ARM_NONE_EABI_INCLUDE}
        -isystem${GCC_ARM_NONE_EABI_INCLUDE}/arm-none-eabi
        -isystem${GCC_ARM_NONE_EABI_INCLUDE}/../../
    )
else()

file(GLOB_RECURSE GCC_ARM_NONE_EABI_INCLUDE
    "${GCC_ARM_NONE_EABI_ROOT}/include/c++/*/cstddef")

get_filename_component(GCC_ARM_NONE_EABI_INCLUDE
    "${GCC_ARM_NONE_EABI_INCLUDE}" DIRECTORY)

add_compile_options(
    -isystem${GCC_ARM_NONE_EABI_INCLUDE}
    -isystem${GCC_ARM_NONE_EABI_INCLUDE}/arm-none-eabi
    -isystem${GCC_ARM_NONE_EABI_INCLUDE}/arm-none-eabi/include
    -isystem${GCC_ARM_NONE_EABI_ROOT}/include
)
endif()

add_compile_options(
    --target=${CLANG_TARGET}
)

# Use GUN linker. Because this project use nano and nosys lib, but lld.ld do not
# support specs files.
set(CMAKE_C_LINK_EXECUTABLE
    "${GNU_COMPILER}-gcc <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc"
)
set(CMAKE_CXX_LINK_EXECUTABLE
    "${GNU_COMPILER}-g++ <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc"
)
