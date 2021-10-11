/**
 * @file ctrl_cap.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 超级电容控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mid_msg_distrib.h"
#include "mod_cap.h"
#include "thd.h"

#define THD_PERIOD_MS (10)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_CtrlCap(void* arg) {
  RM_UNUSED(arg);

  Cap_t cap;
  CAN_CapFeedback_t can_fb;
  CAN_CapOutput_t cap_out;
  Referee_ForCap_t referee_cap;
  UI_CapUI_t cap_ui;

  MsgDist_Publisher_t* out_pub =
      MsgDist_CreateTopic("cap_out", sizeof(CAN_ChassisOutput_t));
  MsgDist_Publisher_t* ui_pub =
      MsgDist_CreateTopic("cap_ui", sizeof(UI_ChassisUI_t));
  MsgDist_Publisher_t* info_pub =
      MsgDist_CreateTopic("cap_info", sizeof(Cap_t));

  MsgDist_Subscriber_t* fb_sub = MsgDist_Subscribe("cap_fb", true);
  MsgDist_Subscriber_t* ref_sub = MsgDist_Subscribe("referee_cap", true);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    MsgDist_Poll(ref_sub, &referee_cap, 0);

    /* 一定时间长度内接收不到电容反馈值，使电容离线 */
    if (MsgDist_Poll(fb_sub, &can_fb, 500) != pdPASS) {
      Cap_HandleOffline(&cap, &cap_out, GAME_CHASSIS_MAX_POWER_WO_REF);
    } else {
      vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      /* 根据裁判系统数据计算输出功率 */
      Cap_Update(&cap, &can_fb);
      Cap_Control(&referee_cap, &cap_out);
      xTaskResumeAll();
    }

    MsgDist_Publish(out_pub, &cap_out);
    MsgDist_Publish(ui_pub, &cap_ui);
    MsgDist_Publish(info_pub, &cap);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
