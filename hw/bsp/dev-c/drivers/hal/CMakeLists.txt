cmake_minimum_required(VERSION 3.11)
project(hal)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${HAL_DIR}/Src/*.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX ".*template.c$")

add_library(${PROJECT_NAME} OBJECT)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
    PRIVATE ${STM32_CMSIS_DIR}/Source/Templates/system_stm32f4xx.c
    PRIVATE ${STM32_CMSIS_DIR}/Source/Templates/gcc/startup_stm32f407xx.s
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${HAL_DIR}/Inc
    PUBLIC  ${STM32_CMSIS_DIR}/Include
    PUBLIC ${ARM_CMSIS_DIR}/CMSIS/Core/Include
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
)
