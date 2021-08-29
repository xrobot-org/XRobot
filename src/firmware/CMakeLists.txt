cmake_minimum_required(VERSION 3.5)
project(firmware)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE hal
    PRIVATE rtos
    PRIVATE thread
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:thread,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME} 
    hal
    rtos
    thread
)

include(utils_gcc)
print_section_sizes(${PROJECT_NAME})
create_bin_output(${PROJECT_NAME})