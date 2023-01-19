set(CMAKE_EXE_LINKER_FLAGS
    "-Wl,--cref,--print-memory-usage,-Map=${CMAKE_PROJECT_NAME}.map"
)

add_link_options(-fstack-usage -fdump-rtl-dfinish)

include($ENV{IDF_PATH}/tools/cmake/toolchain-esp32c3.cmake)
