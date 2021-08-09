cmake_minimum_required(VERSION 3.5)
project(rtos)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME}
    module
    device
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(${PROJECT_NAME} 
    module
)
