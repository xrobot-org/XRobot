project(bsp)

file(GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

include(${MCU_DIR}/linux/driver/CMakeLists.txt)

target_link_directories(
  ${PROJECT_NAME}
  PUBLIC ${WEBOTS_HOME}/lib/controller
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${LIBRARIES}
  PUBLIC ${CMAKE_SHARED_LIBRARY_PREFIX}Controller${CMAKE_SHARED_LIBRARY_SUFFIX}
  PUBLIC ${CMAKE_SHARED_LIBRARY_PREFIX}CppController${CMAKE_SHARED_LIBRARY_SUFFIX}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${WEBOTS_HOME}/include/controller/c
  PUBLIC ${WEBOTS_HOME}/include/controller/cpp
  PUBLIC ${WEBOTS_HOME}/lib/controller
)

# add_dependencies(${PROJECT_NAME})
