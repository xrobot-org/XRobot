execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/project.py generate
                        ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../config/config.cmake)

if(CONFIG_TC_ARM_NONE_EABI)
  include(${CMAKE_CURRENT_LIST_DIR}/arm-none-eabi.cmake)
else()
  message(FATAL_ERROR "No compiler selected.")
endif()
