cmake_minimum_required(VERSION 3.11)
project(device)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

INCLUDE_ALL_DIR(${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_ALL_DIR(${USER_DIR}/device)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC bsp
  PUBLIC component
  PUBLIC system
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${LIB_DIR}/magic_enum/include
  PUBLIC ${${PROJECT_NAME}_INCLUDES}
  PUBLIC $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(
    ${PROJECT_NAME} SYSTEM
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(${PROJECT_NAME} bsp component system OneMessage)
