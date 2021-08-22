cmake_minimum_required(VERSION 3.5)
project(usb)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${THIRD_PARTY_DIR}/stm32_usb/Core/Src/*.c"
    "${THIRD_PARTY_DIR}/stm32_usb/Class/CDC/Src/*.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX ".*template.c$")

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE hal
    PRIVATE stm32_cmsis
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${THIRD_PARTY_DIR}/stm32_usb/Core/Inc
    PUBLIC ${THIRD_PARTY_DIR}/stm32_usb/Class/CDC/Inc
    PRIVATE $<TARGET_PROPERTY:stm32_cmsis,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME} 
    hal
    stm32_cmsis
)
