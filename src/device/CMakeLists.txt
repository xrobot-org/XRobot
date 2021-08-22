cmake_minimum_required(VERSION 3.5)
project(device)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE bsp
    PRIVATE component
    PRIVATE hal
    PRIVATE rtos
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC  ${THIRD_PARTY_DIR}/protocol
    PUBLIC  $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC  $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC  $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    bsp
    component
    hal
    rtos
)
