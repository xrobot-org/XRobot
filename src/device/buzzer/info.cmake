if(${PWM_BUZZER})
    set(DEVICE_BUZZER True PARENT_SCOPE)
    file(GLOB CUR_SOURCES "${SUB_DIR}/*.cpp")

    SUB_ADD_SRC(CUR_SOURCES)
    SUB_ADD_INC(SUB_DIR)
else()
    set(DEVICE_BUZZER False PARENT_SCOPE)
endif()
