cmake_minimum_required(VERSION 3.11)
project(usb)

file(GLOB ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${LIB_DIR}/tinyusb/src/tusb.c"
    "${LIB_DIR}/tinyusb/src/common/tusb_fifo.c"
    "${LIB_DIR}/tinyusb/src/device/usbd.c"
    "${LIB_DIR}/tinyusb/src/device/usbd_control.c"
    "${LIB_DIR}/tinyusb/src/class/cdc/cdc_device.c"
    "${LIB_DIR}/tinyusb/src/class/dfu/dfu_rt_device.c"
    "${LIB_DIR}/tinyusb/src/class/hid/hid_device.c"
    "${LIB_DIR}/tinyusb/src/class/midi/midi_device.c"
    "${LIB_DIR}/tinyusb/src/class/msc/msc_device.c"
    "${LIB_DIR}/tinyusb/src/class/net/net_device.c"
    "${LIB_DIR}/tinyusb/src/class/usbtmc/usbtmc_device.c"
    "${LIB_DIR}/tinyusb/src/class/vendor/vendor_device.c"
    "${LIB_DIR}/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c"
)

list(FILTER ${PROJECT_NAME}_SOURCES EXCLUDE REGEX ".*template.c$")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${LIB_DIR}/tinyusb/src
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
)
