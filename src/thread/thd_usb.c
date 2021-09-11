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

#include "bsp_usb.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#define THD_PERIOD_MS (2)

void Thd_USB(void* arg) {
  Runtime_t* runtime = arg;
  RM_UNUSED(runtime);

  while (1) {
    ;
  }
}
