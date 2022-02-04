cmake_minimum_required(VERSION 3.11)
project(hal)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${THIRD_PARTY_DIR}/stm32_hal/Src/*.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX ".*template.c$")

add_library(${PROJECT_NAME} OBJECT)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
    PRIVATE ${THIRD_PARTY_DIR}/stm32_cmsis/Source/Templates/system_stm32f4xx.c
    PRIVATE ${THIRD_PARTY_DIR}/stm32_cmsis/Source/Templates/gcc/startup_stm32f407xx.s
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${THIRD_PARTY_DIR}/stm32_hal/Inc
    PUBLIC  ${THIRD_PARTY_DIR}/stm32_cmsis/Include
    PUBLIC ${THIRD_PARTY_DIR}/arm_cmsis/CMSIS/Core/Include
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
)
