if(${UART_AI})
    set(DEVICE_AI True PARENT_SCOPE)

    file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")
    set(CUR_INCLUDES ${LIB_DIR}/protocol ${SUB_DIR})

    SUB_ADD_SRC(CUR_SOURCES)
    SUB_ADD_INC(CUR_INCLUDES)
else()
    set(DEVICE_AI False PARENT_SCOPE)
endif()
