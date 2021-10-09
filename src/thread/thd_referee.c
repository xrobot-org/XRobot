/**
 * @file referee.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 裁判系统接收发送线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自裁判系统的数据
 * 解析后根据需求组合成新包发给各个模块
 * 无论裁判系统是否在线，都要按时发送给各个模块
 *
 */

#include "bsp_usb.h"
#include "dev_referee.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

Referee_t ref;
Referee_ForCap_t for_cap;
Referee_ForAI_t for_ai;
Referee_ForChassis_t for_chassis;
Referee_ForLauncher_t for_launcher;

#else

static Referee_t ref;
static Referee_ForCap_t for_cap;
static Referee_ForAI_t for_ai;
static Referee_ForChassis_t for_chassis;
static Referee_ForLauncher_t for_launcher;

#endif

#define THD_PERIOD_MS (1)

void Thd_Referee(void* arg) {
  Runtime_t* runtime = arg;
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* referee_cap_pub =
      MsgDistrib_CreateTopic("referee_cap", sizeof(Referee_ForCap_t));
  MsgDistrib_Publisher_t* referee_ai_pub =
      MsgDistrib_CreateTopic("referee_ai", sizeof(Referee_ForAI_t));
  MsgDistrib_Publisher_t* referee_chassis_pub =
      MsgDistrib_CreateTopic("referee_chassis", sizeof(Referee_ForChassis_t));
  MsgDistrib_Publisher_t* referee_launcher_pub =
      MsgDistrib_CreateTopic("referee_launcher", sizeof(Referee_ForLauncher_t));

  MsgDistrib_Subscriber_t* ui_cap_sub = MsgDistrib_Subscribe("ui_cap", true);
  MsgDistrib_Subscriber_t* ui_chassis_sub =
      MsgDistrib_Subscribe("chassis_ui", true);
  MsgDistrib_Subscriber_t* ui_gimbal_sub =
      MsgDistrib_Subscribe("gimbal_ui", true);
  MsgDistrib_Subscriber_t* ui_launcher_sub =
      MsgDistrib_Subscribe("launcher_ui", true);

  /* 初始化裁判系统 */
  Referee_Init(&ref, &(runtime->cfg.pilot_cfg->screen));

  uint32_t tick = xTaskGetTickCount();
  while (1) {
    Referee_StartReceiving(&ref); /* 开始接收裁判系统数据 */

    /* 判断裁判系统数据是否接收完成 */
    if (Referee_WaitRecvCplt(&ref, 100) != pdTRUE) {
      Referee_HandleOffline(&ref); /* 长时间未接收到数据，裁判系统离线 */
    } else {
      Referee_Parse(&ref); /* 解析裁判系统数据 */
    }

    /* 定时接收发送数据 */
    if (xTaskGetTickCount() > tick) {
      tick += delay_tick;
      /* 打包裁判系统数据 */
      Referee_PackForCap(&for_cap, &ref);
      Referee_PackForAI(&for_ai, &ref);
      Referee_PackForLauncher(&for_launcher, &ref);
      Referee_PackForChassis(&for_chassis, &ref);

      /* 发送裁判系统数据到其他进程 */
      MsgDistrib_Publish(referee_cap_pub, &for_cap);
      MsgDistrib_Publish(referee_ai_pub, &for_ai);
      MsgDistrib_Publish(referee_chassis_pub, &for_chassis);
      MsgDistrib_Publish(referee_launcher_pub, &for_launcher);

      /* 获取其他进程数据用于绘制UI */
      MsgDistrib_Poll(ui_cap_sub, &(ref.cap_ui), 0);
      MsgDistrib_Poll(ui_chassis_sub, &(ref.chassis_ui), 0);
      MsgDistrib_Poll(ui_gimbal_sub, &(ref.gimbal_ui), 0);
      MsgDistrib_Poll(ui_launcher_sub, &(ref.launcher_ui), 0);
      MsgDistrib_Poll(ui_launcher_sub, &(ref.cmd_ui), 0);
#if 0
      xQueueReceive(runtime->msgq.ui.ai, &(ref.ai_ui), 0);
#endif

      /* 刷新UI数据 */
      Referee_RefreshUI(&ref);

      if (Referee_WaitTransCplt(&ref, 0)) {
        Referee_PackUiPacket(&ref);
        Referee_StartTransmit(&ref);
      }
    }
  }
}