cmake_minimum_required(VERSION 3.11)

project(hal)

add_compile_options(-w)

file(GLOB ${PROJECT_NAME}_SOURCES_MX "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c")
file(GLOB ${PROJECT_NAME}_SOURCES_HAL "${HAL_DIR}/Src/*.c")

list(FILTER ${PROJECT_NAME}_SOURCES_HAL EXCLUDE REGEX ".*template.c$")
list(FILTER ${PROJECT_NAME}_SOURCES_MX EXCLUDE REGEX "system_stm32f1xx.c$")

add_library(${PROJECT_NAME} OBJECT)

target_sources(
  ${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_SOURCES_HAL}
  PRIVATE ${${PROJECT_NAME}_SOURCES_MX}
  PRIVATE ${STM32_CMSIS_DIR}/Source/Templates/system_stm32f1xx.c
  PRIVATE ${STM32_CMSIS_DIR}/Source/Templates/gcc/startup_stm32f103xb.s)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
  PUBLIC ${HAL_DIR}/Inc
  PUBLIC ${STM32_CMSIS_DIR}/Include
  PUBLIC ${ARM_CMSIS_DIR}/CMSIS/Core/Include)
