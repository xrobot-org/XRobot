cmake_minimum_required(VERSION 3.5)

#---------------------------------------------------------------------------------------
# stm32_cmsis

add_library(stm32_cmsis STATIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Source/Templates/system_stm32f4xx.c
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Source/Templates/gcc/startup_stm32f407xx.s
)

target_include_directories(stm32_cmsis PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/stm32_cmsis/Include
    ${CMAKE_CURRENT_LIST_DIR}/arm_cmsis/CMSIS/Core/Include
)
