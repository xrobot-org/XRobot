#include "term.hpp"

#include "bsp_sys.h"
#include "bsp_time.h"
#include "bsp_usb.h"

static char printf_buff[1024];

int printf(const char *format, ...) {
  va_list v_arg_list;
  va_start(v_arg_list, format);
  XB_UNUSED(vsnprintf(printf_buff, sizeof(printf_buff), format, v_arg_list));
  va_end(v_arg_list);

  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(printf_buff),
                   strlen(printf_buff));

  return 0;
}
