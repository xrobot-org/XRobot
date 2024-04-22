cmake_minimum_required(VERSION 3.11)
project(rtos)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${LIB_DIR}/freertos/*.c"
    "${LIB_DIR}/freertos/portable/MemMang/heap_4.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX "stream_buffer.c$")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
    PRIVATE ${LIB_DIR}/freertos/portable/GCC/ARM_CM7/r0p1/port.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE hal
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${LIB_DIR}/freertos/include
    PUBLIC  ${LIB_DIR}/freertos/portable/GCC/ARM_CM4F
    PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
)
