cmake_minimum_required(VERSION 3.11)
project(robot)

if(${CONFIG_RB_WHELL_LEG})
  file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/balance_infantry/*.cpp"
  )
  set(ROBOT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/balance_infantry)
elseif(${CONFIG_RB_INFANTRY})
  file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/infantry/*.cpp"
  )
  set(ROBOT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/infantry)
endif()

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE module
  PRIVATE device)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${ROBOT_INC_DIR}
  PUBLIC $<TARGET_PROPERTY:module,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:device,INTERFACE_INCLUDE_DIRECTORIES>)

add_dependencies(${PROJECT_NAME} module device)
