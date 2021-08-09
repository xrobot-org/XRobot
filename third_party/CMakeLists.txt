cmake_minimum_required(VERSION 3.5)

#---------------------------------------------------------------------------------------
# FreeRTOS

file(GLOB ${PROJECT_FREERTOS}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/freertos/*.c"
)

add_library(${PROJECT_FREERTOS} STATIC 
    ${${PROJECT_FREERTOS}_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F/port.c
)

target_include_directories(${PROJECT_FREERTOS} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F/
)

#---------------------------------------------------------------------------------------
# protocol

file(GLOB ${PROJECT_PROTOCOL}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/freertos/*.c"
)

add_library(${PROJECT_PROTOCOL} STATIC ${${PROJECT_PROTOCOL}_SOURCES})

target_include_directories(${PROJECT_PROTOCOL} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)

#---------------------------------------------------------------------------------------
# stm32_cmsis

add_library(${PROJECT_STM32_CMSIS} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_cmsis/Source/Templates/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_cmsis/Source/Templates/gcc/startup_stm32f407xx.s
)

target_include_directories(${PROJECT_STM32_CMSIS} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_cmsis/Include
)

#---------------------------------------------------------------------------------------
# stm32_hal

file(GLOB ${PROJECT_STM32_HAL}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32_hal/Src/*.c"
)

add_library(${PROJECT_STM32_HAL} STATIC ${${PROJECT_STM32_HAL}_SOURCES})

target_include_directories(${PROJECT_STM32_HAL} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_hal/Inc
)

#---------------------------------------------------------------------------------------
# stm32_usb

file(GLOB ${PROJECT_STM32_USB_CLASS}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32_usb/Class/*.c"
)

file(GLOB ${PROJECT_STM32_USB_CORE}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32_usb/Core/Src*.c"
)

add_library(${PROJECT_STM32_USB} STATIC
    ${${PROJECT_STM32_USB_CLASS}_SOURCES}
    ${${PROJECT_STM32_USB_CORE}_SOURCES}
)

target_include_directories(${PROJECT_STM32_USB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_usb/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32_usb/Core/Inc
)