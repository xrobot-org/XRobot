project(bsp)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

include(${MCU_DIR}/default/CMakeLists.txt)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC hal
  PUBLIC flash
  PUBLIC rtos)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:flash,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:rtos,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} hal flash rtos)

add_subdirectory(${BOARD_DIR}/drivers/hal)
add_subdirectory(${BOARD_DIR}/drivers/rtos)
add_subdirectory(${BOARD_DIR}/drivers/flash)
