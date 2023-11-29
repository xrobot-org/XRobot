project(bsp)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_SOURCES}
  PRIVATE bsp_can.cpp)

include(${MCU_DIR}/linux/driver/CMakeLists.txt)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC -lhv
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# add_dependencies(${PROJECT_NAME})
