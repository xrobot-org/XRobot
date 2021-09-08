/**
 * @file thd_msg_distrib.c
 * @author Qu Shen
 * @brief 消息分发线程
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <string.h>

#include "bsp_pwm.h"
#include "bsp_usb.h"
#include "comp_ahrs.h"
#include "comp_pid.h"
#include "dev_bmi088.h"
#include "dev_ist8310.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#define THD_PERIOD_MS (2)

void Thd_MsgDistrib(void *argument) {
  RM_UNUSED(argument); /* 未使用argument，消除警告 */

  /* 初始化消息分发 */
  MsgDistrib_Init();

  while (1) {
    /* 消息分发 */
    MsgDistrib_Distribute();
  }
}
