cmake_minimum_required(VERSION 3.5)

#---------------------------------------------------------------------------------------
# protocol

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/protocol
)

#---------------------------------------------------------------------------------------
# stm32_cmsis

add_library(stm32_cmsis STATIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Source/Templates/system_stm32f4xx.c
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Source/Templates/gcc/startup_stm32f407xx.s
)

target_include_directories(stm32_cmsis PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Include
    ${CMAKE_CURRENT_LIST_DIR}/arm_cmsis/CMSIS/Core/Include
    ${CMAKE_CURRENT_LIST_DIR}/arm_cmsis/CMSIS/RTOS2/Include
)

#---------------------------------------------------------------------------------------
# stm32_usb

add_library(stm32_usb STATIC
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Class/CDC/Src/usbd_cdc.c"
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Src/usbd_core.c"
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Src/usbd_ctlreq.c"
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Src/usbd_ioreq.c"
)

target_include_directories(stm32_usb PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Class/CDC/Inc
    ${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Inc
)