cmake_minimum_required(VERSION 3.11)
project(module)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

INCLUDE_ALL_DIR(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES} module.hpp)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE component
  PRIVATE device)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${${PROJECT_NAME}_INCLUDES}
  PUBLIC ${LIB_DIR}/magic_enum/include
  PUBLIC $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>)

target_include_directories(
    ${PROJECT_NAME} SYSTEM
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(${PROJECT_NAME} component device)
