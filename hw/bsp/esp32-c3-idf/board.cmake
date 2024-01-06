cmake_minimum_required(VERSION 3.16)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
# Create idf::{target} and idf::freertos static libraries
idf_build_process("esp32c3"
                # try and trim the build; additional components
                # will be included as needed based on dependency tree
                #
                # although esptool_py does not generate static library,
                # processing the component is needed for flashing related
                # targets and file generation
                COMPONENTS freertos esptool_py driver nvs_flash bt
                SDKCONFIG ${BOARD_DIR}/sdkconfig
                BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options()

set(elf_file ${CMAKE_PROJECT_NAME}.elf)
add_executable(${elf_file} ${BOARD_DIR}/main.cpp)

# Link the static libraries to the executable
target_link_libraries(
  ${elf_file}
  idf::bt
  idf::freertos
  idf::spi_flash
  idf::driver
  idf::nvs_flash
  bsp
  system
  robot
)

add_subdirectory(${BOARD_DIR}/drivers)

target_include_directories(
  ${PROJECT_NAME}.elf
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:robot,INTERFACE_INCLUDE_DIRECTORIES>
)

idf_build_executable(${elf_file})
