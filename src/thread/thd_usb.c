/**
 * @file thd_usb.c
 * @author Qu Shen
 * @brief USB输入输出管理
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <string.h>

#include "thd.h"
#include "tusb.h"

#define THD_PERIOD_MS (2)

void Thd_USB(void* arg) {
  Runtime_t* runtime = arg;
  RM_UNUSED(runtime);

  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS
  // queue API.
  tusb_init();

  // RTOS forever loop
  while (1) {
    // tinyusb device task
    tud_task();
  }
}
THREAD_DECLEAR(Thd_USB, 128, 4);
