cmake_minimum_required(VERSION 3.11)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/component)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/device)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/middleware)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thread)

add_executable(${PROJECT_NAME}.elf)

target_sources(${PROJECT_NAME}.elf
    PRIVATE main.c
)

target_link_libraries(${PROJECT_NAME}.elf
    PRIVATE hal
    PRIVATE rtos
    PRIVATE thread
)

target_include_directories(${PROJECT_NAME}.elf
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:thread,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}.elf
    thread
)
