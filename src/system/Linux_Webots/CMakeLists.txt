target_include_directories(
  OneMessage
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Linux_Webots
  PRIVATE $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>)

target_include_directories(
  MiniShell
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Linux_Webots
  PRIVATE $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Linux_Webots/*.cpp")

add_library(${PROJECT_NAME} OBJECT)

target_sources(${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_directories(
  ${PROJECT_NAME}
  PUBLIC ${WEBOTS_HOME}/lib/controller
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE bsp
  PRIVATE OneMessage
  PRIVATE MiniShell
  PRIVATE stdc++
  PUBLIC ${CMAKE_SHARED_LIBRARY_PREFIX}Controller${CMAKE_SHARED_LIBRARY_SUFFIX}
  PUBLIC ${CMAKE_SHARED_LIBRARY_PREFIX}CppController${CMAKE_SHARED_LIBRARY_SUFFIX}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Linux_Webots
  PUBLIC $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:OneMessage,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC $<TARGET_PROPERTY:MiniShell,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC ${WEBOTS_HOME}/include/controller/c
  PUBLIC ${WEBOTS_HOME}/include/controller/cpp
  PUBLIC ${WEBOTS_HOME}/lib/controller
)

target_include_directories(
  ${PROJECT_NAME} SYSTEM
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Linux_Webots
)

add_dependencies(
  ${PROJECT_NAME}
  bsp
  OneMessage)
