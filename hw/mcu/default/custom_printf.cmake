set(SUPPORT_EXPONENTIAL_SPECIFIERS 0)
set(MSVC_STYLE_INTEGER_SPECIFIERS 0)
set(SUPPORT_WRITEBACK_SPECIFIER 0)
set(BUILD_STATIC_LIBRARY 1)

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
