cmake_minimum_required(VERSION 3.11)
project(rtos)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${LIB_DIR}/freertos/croutine.c"
    "${LIB_DIR}/freertos/event_groups.c"
    "${LIB_DIR}/freertos/list.c"
    "${LIB_DIR}/freertos/queue.c"
    "${LIB_DIR}/freertos/tasks.c"
    "${LIB_DIR}/freertos/timers.c"
    "${LIB_DIR}/freertos/portable/MemMang/heap_4.c"
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
    PRIVATE ${LIB_DIR}/freertos/portable/GCC/ARM_CM3/port.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE hal
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${LIB_DIR}/freertos/include
    PUBLIC  ${LIB_DIR}/freertos/portable/GCC/ARM_CM3
    PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
)
