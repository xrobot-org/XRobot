set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

add_compile_options(--target=${CLANG_TARGET} --sysroot=${CLANG_SYS_ROOT})

# Use GUN linker. Because this project use nano and nosys lib, but lld.ld do not
# support specs files.
set(CMAKE_C_LINK_EXECUTABLE
    "arm-none-eabi-gcc <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc"
)
set(CMAKE_CXX_LINK_EXECUTABLE
    "arm-none-eabi-g++ <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc"
)
