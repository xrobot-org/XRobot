cmake_minimum_required(VERSION 3.11)
project(robot)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}robot-${child}})
    set(ROBOT_DIR ${child})
    set(ROBOT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${ROBOT_DIR})
    file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${ROBOT_DIR}/*.cpp")
  ENDIF()
ENDFOREACH()

FILE(GLOB children RELATIVE ${USER_DIR}/robot ${USER_DIR}/robot/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}robot-${child}})
    set(ROBOT_DIR ${child})
    file(GLOB ${PROJECT_NAME}_SOURCES "${USER_DIR}/robot/${ROBOT_DIR}/*.cpp")
    set(ROBOT_INC_DIR ${USER_DIR}/robot/${ROBOT_DIR})
  ENDIF()
ENDFOREACH()


add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE system
  PRIVATE module
  PRIVATE device
  PRIVATE component)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${ROBOT_INC_DIR}
  PUBLIC $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:module,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} system module device component)
