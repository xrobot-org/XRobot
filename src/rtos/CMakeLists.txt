cmake_minimum_required(VERSION 3.5)
project(rtos)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${THIRD_PARTY_DIR}/freertos/*.c"
    "${THIRD_PARTY_DIR}/freertos/portable/MemMang/heap_4.c"
)

add_library(${PROJECT_NAME} STATIC 
    ${${PROJECT_NAME}_SOURCES}
    ${THIRD_PARTY_DIR}/freertos/portable/GCC/ARM_CM4F/port.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${THIRD_PARTY_DIR}/freertos/include
    PUBLIC  ${THIRD_PARTY_DIR}/freertos/portable/GCC/ARM_CM4F
)
