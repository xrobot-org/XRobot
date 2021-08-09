# Target definition
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

#---------------------------------------------------------------------------------------
message(STATUS "--------------------------- Tool Chain -------------------------------")

# Tool chain path
set(TOOLCHAIN arm-none-eabi)

if(NOT DEFINED TOOLCHAIN_DIR)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
        set(TOOLCHAIN_DIR "/usr")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
        set(TOOLCHAIN_DIR "/usr/local")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
        message(STATUS "Please specify the TOOLCHAIN_DIR !\n For example: -DTOOLCHAIN_PREFIX=\"C:/Program Files/GNU Tools ARM Embedded\" ")
    else()
        set(TOOLCHAIN_DIR "/usr")
        message(STATUS "No TOOLCHAIN_DIR specified, using default: " ${TOOLCHAIN_DIR})
    endif()
endif() 

set(TOOLCHAIN_BIN_DIR ${TOOLCHAIN_DIR}/bin)
set(TOOLCHAIN_INC_DIR ${TOOLCHAIN_DIR}/${TOOLCHAIN}/include)
set(TOOLCHAIN_LIB_DIR ${TOOLCHAIN_DIR}/${TOOLCHAIN}/lib)

# Set system depended extensions
if(WIN32)
    set(TOOLCHAIN_EXT ".exe" )
else()
    set(TOOLCHAIN_EXT "" )
endif()

#---------------------------------------------------------------------------------------
# General Compiler Options

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options for DEBUG build
# -Og enables optimizations that do not interfere with debugging
# -g produce debugging information in the operating system’s native format
set(CMAKE_C_FLAGS_DEBUG "-Og -g -gdwarf")
set(CMAKE_ASM_FLAGS_DEBUG "-g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

# Options for RELEASE build
# -Os Optimize for size. -Os enables all -O2 optimizations
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

#---------------------------------------------------------------------------------------
# Set compilers
set(CMAKE_C_COMPILER ${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN}-gcc${TOOLCHAIN_EXT})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN}-gcc${TOOLCHAIN_EXT})
set(CMAKE_SIZE ${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN}-size${TOOLCHAIN_EXT})
set(CMAKE_OBJCOPY ${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN}-objcopy${TOOLCHAIN_EXT})
set(CMAKE_OBJDUMP ${TOOLCHAIN_BIN_DIR}/${TOOLCHAIN}-objdump${TOOLCHAIN_EXT})
