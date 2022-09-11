if(${UART_DR16})
    set(DEVICE_DR16 True PARENT_SCOPE)
    file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")

    SUB_ADD_SRC(CUR_SOURCES)
    SUB_ADD_INC(SUB_DIR)
else()
    set(DEVICE_DR16 False PARENT_SCOPE)
endif()
