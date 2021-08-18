cmake_minimum_required(VERSION 3.5)
project(hal)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${THIRD_PARTY_DIR}/stm32_hal/Src/*.c" 
)

add_library(${PROJECT_NAME} STATIC 
    ${${PROJECT_NAME}_SOURCES}
    ${STM32_HAL_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    stm32_cmsis
    rtos
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${THIRD_PARTY_DIR}/stm32_hal/Inc
    PRIVATE $<TARGET_PROPERTY:stm32_cmsis,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME} 
    stm32_cmsis
    rtos
)
