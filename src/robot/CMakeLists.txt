cmake_minimum_required(VERSION 3.11)
project(robot)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

FOREACH(child ${children})
  IF(${_SUB_CFG_${child}})
    set(ROBOT_DIR ${child})
  ENDIF()
ENDFOREACH()

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${ROBOT_DIR}/*.cpp")
set(ROBOT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${ROBOT_DIR})

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE module
  PRIVATE device)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${ROBOT_INC_DIR}
  PUBLIC $<TARGET_PROPERTY:module,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} module device)
