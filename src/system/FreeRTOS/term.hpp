#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "bsp_usb.h"

namespace System {
class Term {
 public:
  Term();

  static bool Opened() { return bsp_usb_connect(); }

  static uint32_t Available() { return bsp_usb_avail(); }

  static char ReadChar() { return bsp_usb_read_char(); }

  static uint32_t Read(uint8_t *buffer, uint32_t len) {
    return bsp_usb_read(buffer, len);
  }

  static bool Write(uint8_t *buffer, uint32_t len) {
    return bsp_usb_transmit(buffer, len) == BSP_OK;
  }
};
}  // namespace System
