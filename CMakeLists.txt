if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

add_compile_definitions(
    BOARD_RM_C
    STM32F407xx
    HSE_VALUE=12000000
    HSE_STARTUP_TIMEOUT=100
)

cmake_minimum_required(VERSION 3.11)
# 版本第一位与赛季年份对应，同时与branch对应
# 第二位代表功能变化
# 第三位代表bug修复
project(qdu_rm_mcu
    DESCRIPTION "MCU for Robomaster"
    VERSION 2022.0.0
    LANGUAGES C ASM
)
#---------------------------------------------------------------------------------------
# Add config file
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/project.py generate ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config)
include(config)

#---------------------------------------------------------------------------------------
# LIB
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

#---------------------------------------------------------------------------------------
# Hardware
set(HW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw)
set(CMAKE_MODULE_PATH ${HW_DIR})
include(hw)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options for DEBUG build
if(CONFIG_FW_DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -ggdb3")
    set(CMAKE_ASM_FLAGS_DEBUG "-g3 -ggdb3")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Options for RELEASE build
if(CONFIG_FW_RELEASE)
    set(CMAKE_C_FLAGS_RELEASE "-Os")
    set(CMAKE_ASM_FLAGS_RELEASE "")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
    set(CMAKE_BUILD_TYPE "Realase")
endif()

set(CMAKE_C_FLAGS $ENV{HW_C_FLAGS})
set(CMAKE_ASM_FLAGS $ENV{HW_ASM_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS $ENV{HW_LINK_FLAGS})

# message(FATAL_ERROR ${HW_COMPILER_DEFINITIONS})

#---------------------------------------------------------------------------------------
# Building Options

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
        MCU_DEBUG_BUILD
        USE_FULL_ASSERT
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(
        MCU_DEBUG_BUILD
        USE_FULL_ASSERT
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Realase")
    add_compile_definitions()
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_definitions()
endif()

#---------------------------------------------------------------------------------------
# SOURCE


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
#---------------------------------------------------------------------------------------
# Test

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

#---------------------------------------------------------------------------------------
