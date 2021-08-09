if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

cmake_minimum_required(VERSION 3.5)

# 版本第一位与赛季年份对应，同时与branch对应
# 第二位代表功能变化
# 第三位代表bug修复
project(qdu_rm_mcu
    DESCRIPTION "MCU for Robomaster"
    VERSION 2022.0.0
    LANGUAGES C ASM
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

#---------------------------------------------------------------------------------------

include(cmake/toochain)
include(cmake/device_stm32f4)

#---------------------------------------------------------------------------------------
# Linker script

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F407IGHx_FLASH.ld)

#---------------------------------------------------------------------------------------
message(STATUS "------------------------- Host Platform ------------------------------")

string(TIMESTAMP CONFIG_TIMESTAMP "" UTC)
message(STATUS "Timestamp: ${CONFIG_TIMESTAMP}")
message(STATUS "Host: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(CMAKE_CROSSCOMPILING) # set to true by CMake if the CMAKE_SYSTEM_NAME has been set manually 
    message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "CMake: ${CMAKE_VERSION}")
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
message(STATUS "CMake build tool: ${CMAKE_BUILD_TOOL}")

if(MSVC)
    message(STATUS "MSVC: ${MSVC_VERSION}")
endif()
    
if(CMAKE_GENERATOR MATCHES Xcode)
    message(STATUS "Xcode: ${XCODE_VERSION}")
endif()
if(NOT CMAKE_GENERATOR MATCHES "Xcode|Visual Studio")
    message(STATUS "Configuration: ${CMAKE_BUILD_TYPE}")
endif()

#---------------------------------------------------------------------------------------
message(STATUS "---------------------- Compiler Linker option ------------------------")

message(STATUS "C standard: ${CMAKE_C_STANDARD}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C flags (Debug): ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "C flags (Release): ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
if(WIN32)
    message(STATUS "Linker flags (Release): ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    message(STATUS "Linker flags (Debug): ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
else()
    message(STATUS "Linker flags (Release): ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
    message(STATUS "Linker flags (Debug): ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
endif()
message("Linker script: ${LINKER_SCRIPT}")

#---------------------------------------------------------------------------------------
# Building Options

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Realase")
    add_compile_definitions()
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_definitions()
endif ()

#---------------------------------------------------------------------------------------
# Source

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

#---------------------------------------------------------------------------------------
# Test

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

#---------------------------------------------------------------------------------------
message(STATUS "-------------------------------- END ---------------------------------")
