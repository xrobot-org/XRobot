cmake_minimum_required(VERSION 3.11)
project(component)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

INCLUDE_ALL_DIR(${USER_DIR}/component)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE m
    PRIVATE system
    PRIVATE bsp)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>)

target_include_directories(
    ${PROJECT_NAME} SYSTEM
    PUBLIC ${${PROJECT_NAME}_INCLUDES}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${LIB_DIR}/eigen/Eigen)

add_compile_definitions(EIGEN_NO_MALLOC)

add_dependencies(${PROJECT_NAME} OneMessage system)
