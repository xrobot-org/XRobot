cmake_minimum_required(VERSION 3.16)

include_directories(
  PUBLIC $ENV{IDF_PATH}/components/esp_http_client/include
  PUBLIC $ENV{IDF_PATH}/components/mdns/include
  PUBLIC $ENV{IDF_PATH}/components/lwip/lwip/src/include/lwip/apps
  PUBLIC $ENV{IDF_PATH}/components/libhelix-mp3/libhelix-mp3/pub
)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

add_compile_definitions(ARDUINO_USB_CDC_ON_BOOT=1 ARDUINO_USB_MODE=1)

# Create idf::{target} and idf::freertos static libraries
idf_build_process("esp32c3"
                # try and trim the build; additional components
                # will be included as needed based on dependency tree
                #
                # although esptool_py does not generate static library,
                # processing the component is needed for flashing related
                # targets and file generation
                COMPONENTS freertos esptool_py driver nvs_flash bt mdns libhelix-mp3 esp_http_client esp_https_ota arduino-esp32
                SDKCONFIG ${BOARD_DIR}/sdkconfig
                BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
  idf::mdns
  idf::libhelix-mp3
  idf::esp_wifi
  idf::esp_http_client
  idf::esp_https_ota
  idf::arduino-esp32
  bsp
  system
  robot
)

add_subdirectory(${BOARD_DIR}/drivers)


target_include_directories(
  ${PROJECT_NAME}.elf
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC $<TARGET_PROPERTY:idf::mdns,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:robot,INTERFACE_INCLUDE_DIRECTORIES>
)

idf_build_executable(${elf_file})
