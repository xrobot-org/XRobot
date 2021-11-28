cmake_minimum_required(VERSION 3.5)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bsp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/component)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/device)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/hal)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/middleware)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtos)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thread)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/usb)

add_executable(${PROJECT_NAME}.elf main.c)

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
    hal
    rtos
    thread
)

include(toolchain_utils)

print_section_sizes(${PROJECT_NAME}.elf)

create_hex_output(${PROJECT_NAME})
create_bin_output(${PROJECT_NAME})
