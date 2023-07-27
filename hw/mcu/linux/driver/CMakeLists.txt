include(${MCU_DIR}/default/CMakeLists.txt)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${MCU_DIR}/linux/driver)

file(GLOB ${PROJECT_NAME}_SOURCES "${MCU_DIR}/linux/driver/*.c")

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})
