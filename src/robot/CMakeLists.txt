cmake_minimum_required(VERSION 3.11)
project(robot)

if(CONFIG_ID_INFANTRY)
  file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/balance_infantry/*.cpp"
  )
endif()

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE module
  PRIVATE device)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/balance_infantry
  PUBLIC $<TARGET_PROPERTY:module,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} module device)
