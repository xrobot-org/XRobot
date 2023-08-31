if(NOT DEFINED SUPPORT_EXPONENTIAL_SPECIFIERS)
  set(SUPPORT_EXPONENTIAL_SPECIFIERS 0)
endif()

if(NOT DEFINED MSVC_STYLE_INTEGER_SPECIFIERS)
  set(MSVC_STYLE_INTEGER_SPECIFIERS 0)
endif()

if(NOT DEFINED SUPPORT_WRITEBACK_SPECIFIER)
  set(SUPPORT_WRITEBACK_SPECIFIER 0)
endif()

if(NOT DEFINED BUILD_STATIC_LIBRARY)
  set(BUILD_STATIC_LIBRARY 0)
endif()

if(NOT DEFINED SUPPORT_DECIMAL_SPECIFIERS)
  set(SUPPORT_DECIMAL_SPECIFIERS 1)
endif()

if(NOT DEFINED PRINTF_SUPPORT_WRITEBACK_SPECIFIER)
  set(PRINTF_SUPPORT_WRITEBACK_SPECIFIER 0)
endif()

if(NOT DEFINED PRINTF_SUPPORT_LONG_LONG)
  set(PRINTF_SUPPORT_LONG_LONG 0)
endif()

add_subdirectory(${LIB_DIR}/printf printf.o)

add_compile_options(
  -fno-builtin-printf
  -fno-builtin-sprintf
  -fno-builtin-vsprintf
  -fno-builtin-snprintf
  -fno-builtin-vsnprintf
  -fno-builtin-vprintf
)

target_compile_options(printf PRIVATE -w)

link_libraries(printf)
