if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

cmake_minimum_required(VERSION 3.11)

# 版本第一位与年份对应 第二位代表功能变化 第三位代表bug修复
project(
  xrobot
  DESCRIPTION "XRobot Build"
  VERSION 2024.1.1
  LANGUAGES C CXX ASM)

# ---------------------------------------------------------------------------------------
# Default Options
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

# ---------------------------------------------------------------------------------------
# Tools
include(${CMAKE_CURRENT_SOURCE_DIR}/utils/CMake/tools.cmake)

# ---------------------------------------------------------------------------------------
# Config
include(${CMAKE_CURRENT_SOURCE_DIR}/config/config.cmake)

# ---------------------------------------------------------------------------------------
# Building Options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS_DEBUG "-Og -g")
  set(CMAKE_ASM_FLAGS_DEBUG "-Og -g")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
  add_compile_definitions(MCU_DEBUG_BUILD)
  add_compile_options(-Werror -gdwarf-4)

elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
  set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "-O3 -g")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "")
  add_compile_definitions(MCU_DEBUG_BUILD)
  add_compile_options(-Werror -gdwarf-4)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_ASM_FLAGS_RELEASE "-O3")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
  add_compile_definitions(EIGEN_NO_DEBUG=1)

elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
  set(CMAKE_ASM_FLAGS_MINSIZEREL "-Os")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "")
  add_compile_definitions(EIGEN_NO_DEBUG=1)
else()
  message(FATAL_ERROR "Unknown build type.")
endif()

# ---------------------------------------------------------------------------------------
# Library
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# ---------------------------------------------------------------------------------------
# User
set(USER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/user)
set(USER_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/user/lib)

# ---------------------------------------------------------------------------------------
# Hardware
set(HW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw)
include(${HW_DIR}/hw.cmake)

# ---------------------------------------------------------------------------------------
# Source
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------------------
# Test add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

# ---------------------------------------------------------------------------------------
# Build information
message(
  STATUS
  "------------------------- Host Platform ------------------------------")

string(TIMESTAMP CONFIG_TIMESTAMP "" UTC)
message(STATUS "Timestamp: ${CONFIG_TIMESTAMP}")
message(
  STATUS
  "Host: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR}"
)

if(CMAKE_CROSSCOMPILING) # set to true by CMake if the CMAKE_SYSTEM_NAME has
  # been set manually
  message(
    STATUS
    "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}"
  )
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

# ---------------------------------------------------------------------------------------
message(
  STATUS
  "---------------------- Compiler Linker option ------------------------")

message(STATUS "C standard: ${CMAKE_C_STANDARD}")
message(STATUS "")
message(STATUS "C Compiler:     ${CMAKE_C_COMPILER}")
message(STATUS "ASM Compiler:   ${CMAKE_ASM_COMPILER}")
message(STATUS "Linker:         ${CMAKE_LINKER}")
message(STATUS "")
message(STATUS "C flags (General):  ${CMAKE_C_FLAGS}")
message(STATUS "C flags (Debug):    ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "C flags (Release):  ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "")
message(STATUS "ASM flags (General):    ${CMAKE_ASM_FLAGS}")
message(STATUS "ASM flags (Debug):      ${CMAKE_ASM_FLAGS_DEBUG}")
message(STATUS "ASM flags (Release):    ${CMAKE_ASM_FLAGS_RELEASE}")
message(STATUS "")
message(STATUS "Linker flags (General): ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "Linker flags (Debug):   ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
message(STATUS "Linker flags (Release): ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(STATUS "")
message(STATUS "Linker script: ${LINKER_SCRIPT}")
