cmake_minimum_required(VERSION 3.5)
project(app)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE hal
    PRIVATE rtos
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME} 
    hal
    rtos
)