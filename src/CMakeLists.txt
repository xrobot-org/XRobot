cmake_minimum_required(VERSION 3.11)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/component)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/device)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/middleware)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/system)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/robot)

add_executable(${PROJECT_NAME}.elf)

target_sources(${PROJECT_NAME}.elf PRIVATE main.cpp)

target_link_libraries(
  ${PROJECT_NAME}.elf
  PRIVATE bsp
  PRIVATE robot
  PRIVATE system)

target_include_directories(
  ${PROJECT_NAME}.elf
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE $<TARGET_PROPERTY:bsp,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:robot,INTERFACE_INCLUDE_DIRECTORIES>
  PRIVATE $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>)
