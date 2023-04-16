#include "bsp.h"

#include "bsp_time.h"
#include "bsp_uart.h"

void bsp_init() {
  bsp_uart_init();
  bsp_time_init();
}
