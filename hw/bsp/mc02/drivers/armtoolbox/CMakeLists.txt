cmake_minimum_required(VERSION 3.11)
project(armtool)

add_compile_options(-w)


file(GLOB ${PROJECT_NAME}_SOURCES "${LIB_DIR}/arm_toolbox/*.cpp")

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
PRIVATE ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE hal
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${LIB_DIR}/arm_toolbox
  PRIVATE $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
  )

add_dependencies(${PROJECT_NAME}
    hal
)
