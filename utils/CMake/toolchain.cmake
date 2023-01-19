execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/project.py generate
  ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../../config/config.cmake)

set(BSP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw/bsp)
set(MCU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw/mcu)
set(TOOLCHAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/toolchain)

FILE(GLOB children RELATIVE ${BSP_DIR} ${BSP_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    include(${BSP_DIR}/${child}/toolchain.cmake)
  ENDIF()
ENDFOREACH()
