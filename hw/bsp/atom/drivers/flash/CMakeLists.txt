cmake_minimum_required(VERSION 3.11)

add_subdirectory(${LIB_DIR}/MiniFlashDB mf.out)

project(flash)

add_compile_options(-w)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE ${${PROJECT_NAME}_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC hal
    PUBLIC MiniFlash
)

target_include_directories(MiniFlash
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:hal,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC $<TARGET_PROPERTY:MiniFlash,INTERFACE_INCLUDE_DIRECTORIES>
)

add_dependencies(${PROJECT_NAME}
    hal
    MiniFlash
)
