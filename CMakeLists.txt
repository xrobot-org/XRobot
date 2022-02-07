if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

cmake_minimum_required(VERSION 3.11)

# 版本第一位与赛季年份对应，同时与branch对应 第二位代表功能变化 第三位代表bug修复
project(
  qdu_rm_mcu
  DESCRIPTION "MCU for Robomaster"
  VERSION 2022.0.0
  LANGUAGES C ASM)
# ---------------------------------------------------------------------------------------
# Add config file
include(${CMAKE_CURRENT_SOURCE_DIR}/config/config.cmake)

# ---------------------------------------------------------------------------------------
# Library
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# ---------------------------------------------------------------------------------------
# Hardware
set(HW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw)
include(${HW_DIR}/hw.cmake)

# ---------------------------------------------------------------------------------------
# Default Options
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options for DEBUG build
if(CONFIG_FW_DEBUG)
  set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -ggdb3")
  set(CMAKE_ASM_FLAGS_DEBUG "-g3 -ggdb3")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
  set(CMAKE_BUILD_TYPE "Debug")

  add_compile_definitions(MCU_DEBUG_BUILD USE_FULL_ASSERT)
endif()

# Options for RELEASE build
if(CONFIG_FW_RELEASE)
  set(CMAKE_C_FLAGS_RELEASE "-Os")
  set(CMAKE_ASM_FLAGS_RELEASE "")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
  set(CMAKE_BUILD_TYPE "Realase")
endif()

# ---------------------------------------------------------------------------------------
# SOURCE
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------------------
# Test add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

# ---------------------------------------------------------------------------------------
