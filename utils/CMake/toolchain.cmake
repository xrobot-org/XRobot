include(${CMAKE_CURRENT_LIST_DIR}/../../config/config.cmake)

set(BSP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw/bsp)
set(MCU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hw/mcu)
set(USER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/user)
set(TOOLCHAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/toolchain)

FILE(GLOB children RELATIVE ${BSP_DIR} ${BSP_DIR}/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    include(${BSP_DIR}/${child}/toolchain.cmake)
  ENDIF()
ENDFOREACH()

FILE(GLOB children RELATIVE ${USER_DIR}/bsp ${USER_DIR}/bsp/*)

FOREACH(child ${children})
  IF(${${CONFIG_PREFIX}board-${child}})
    include(${USER_DIR}/bsp/${child}/toolchain.cmake)
  ENDIF()
ENDFOREACH()
