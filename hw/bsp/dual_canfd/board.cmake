cmake_minimum_required(VERSION 3.11)

add_compile_definitions(STM32G0B1xx USE_FULL_LL_DRIVER)

set(HAL_DIR ${MCU_DIR}/st/stm32g0xx_hal_driver)
set(STM32_CMSIS_DIR ${MCU_DIR}/st/cmsis_device_g0)
set(ARM_CMSIS_DIR ${LIB_DIR}/cmsis_5)

set(SUPPORT_DECIMAL_SPECIFIERS 0)
include(${MCU_DIR}/st/cmake/mcu_stm32g0.cmake)
include(${MCU_DIR}/st/cmake/toolchain_utils.cmake)

add_subdirectory(${BOARD_DIR}/drivers)

add_executable(${PROJECT_NAME}.elf ${BOARD_DIR}/main.cpp)

target_link_libraries(
  ${PROJECT_NAME}.elf
  PUBLIC bsp
  PUBLIC system
  PUBLIC robot)


target_include_directories(
  ${PROJECT_NAME}.elf
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:robot,INTERFACE_INCLUDE_DIRECTORIES>
)

create_hex_output(${PROJECT_NAME})
create_bin_output(${PROJECT_NAME})
print_section_sizes(${PROJECT_NAME}.elf)
