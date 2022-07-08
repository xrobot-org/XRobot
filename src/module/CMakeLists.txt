cmake_minimum_required(VERSION 3.11)
project(module)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE component
  PRIVATE device)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC $<TARGET_PROPERTY:component,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} component device)
