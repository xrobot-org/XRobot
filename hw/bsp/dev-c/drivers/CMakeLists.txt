add_subdirectory(${BOARD_DIR}/drivers/hal)
add_subdirectory(${BOARD_DIR}/drivers/rtos)
add_subdirectory(${BOARD_DIR}/drivers/usb)

project(bsp)

file(GLOB ${PROJECT_NAME}_BOARD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/drivers/*.c"
)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_BOARD_SOURCES}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC hal
    PRIVATE component
    PUBLIC rtos
    PUBLIC usb
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${BSP_DIR}
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:usb,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
    component
    rtos
    usb
)
