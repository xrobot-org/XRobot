cmake_minimum_required(VERSION 3.5)

#---------------------------------------------------------------------------------------
# FreeRTOS

file(GLOB FREERTOS_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/freertos/*.c"
)

add_library(freertos STATIC 
    ${FREERTOS_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/freertos/portable/GCC/ARM_CM4F/port.c
)

target_include_directories(freertos PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/freertos/include
    ${CMAKE_CURRENT_LIST_DIR}/freertos/portable/GCC/ARM_CM4F/
)

#---------------------------------------------------------------------------------------
# protocol

file(GLOB protocol_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/protocol/*.c"
)

add_library(protocol STATIC ${${PROJECT_PROTOCOL}_SOURCES})

target_include_directories(protocol PUBLIC
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
)

#---------------------------------------------------------------------------------------
# stm32_hal

file(GLOB stm32_hal_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/stm32_hal/Src/*.c"
)

add_library(stm32_hal STATIC ${stm32_hal_SOURCES})

target_include_directories(stm32_hal PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_hal/Inc
)

#---------------------------------------------------------------------------------------
# stm32_usb

file(GLOB STM32_USB_CLASS_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Class/*.c"
)

file(GLOB STM32_USB_CORE_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Src*.c"
)

add_library(stm32_usb STATIC
    ${STM32_USB_CLASS_SOURCES}
    ${STM32_USB_CORE_SOURCES}
)

target_include_directories(stm32_usb PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Class/CDC/Inc
    ${CMAKE_CURRENT_LIST_DIR}/stm32_usb/Core/Inc
)