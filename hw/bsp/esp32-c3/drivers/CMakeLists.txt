project(bsp)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

# target_link_libraries(${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC $<TARGET_PROPERTY:idf::freertos,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:idf::driver,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:idf::nvs_flash,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:idf::esp_timer,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $ENV{IDF_PATH}/components/freertos/include/freertos
  PUBLIC $ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos
)

# add_dependencies(${PROJECT_NAME})
