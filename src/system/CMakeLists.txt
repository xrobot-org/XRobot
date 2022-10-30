cmake_minimum_required(VERSION 3.11)

add_subdirectory(${LIB_DIR}/one-message om.out)
add_subdirectory(${LIB_DIR}/mini_shell ms.out)

project(system)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}${child}})
    include(${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
  ENDIF()
ENDFOREACH()
