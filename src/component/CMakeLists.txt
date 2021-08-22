cmake_minimum_required(VERSION 3.5)
project(component)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})

# target_link_libraries(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} 
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

# 不应该依赖于任何其他库
# add_dependencies(${PROJECT_NAME})