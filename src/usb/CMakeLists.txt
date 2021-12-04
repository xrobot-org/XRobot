cmake_minimum_required(VERSION 3.11)
project(usb)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/tusb.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/common/tusb_fifo.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/device/usbd.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/device/usbd_control.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/cdc/cdc_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/dfu/dfu_rt_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/hid/hid_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/midi/midi_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/msc/msc_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/net/net_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/usbtmc/usbtmc_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/class/vendor/vendor_device.c"
    "${THIRD_PARTY_DIR}/tinyusb/src/portable/st/synopsys/dcd_synopsys.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX ".*template.c$")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE rtos
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${THIRD_PARTY_DIR}/tinyusb/src
    PUBLIC $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
    rtos
)
