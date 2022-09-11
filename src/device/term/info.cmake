if(${USB_CDC})
    set(DEVICE_TERM True PARENT_SCOPE)
    file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")

    SUB_ADD_SRC(CUR_SOURCES)
    SUB_ADD_INC(SUB_DIR)
else()
    set(DEVICE_TERM False PARENT_SCOPE)
endif()
