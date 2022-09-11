if(${BSP_CAN})
    file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")

    SUB_ADD_SRC(CUR_SOURCES)
    SUB_ADD_INC(SUB_DIR)
endif()
