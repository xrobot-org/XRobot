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

static Cap_t cap;

#ifdef MCU_DEBUG_BUILD

CAN_CapFeedback_t can_fb;
CAN_CapOutput_t cap_out;
Referee_ForCap_t referee_cap;
UI_CapUI_t cap_ui;

#else

static CAN_CapFeedback_t can_fb;
static CAN_CapOutput_t cap_out;
static Referee_ForCap_t referee_cap;
static UI_CapUI_t cap_ui;

#endif

/**
 * @brief 控制电容
 *
 * @param argument 未使用
 */
void Thread_CtrlCap(void* argument) {
  Runtime_t* runtime = argument;
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CTRL_CAP);

  MsgDistrib_Publisher_t* out_pub =
      MsgDistrib_CreateTopic("cap_out", sizeof(CAN_ChassisOutput_t));
  MsgDistrib_Publisher_t* ui_pub =
      MsgDistrib_CreateTopic("cap_ui", sizeof(UI_ChassisUI_t));
  MsgDistrib_Publisher_t* info_pub =
      MsgDistrib_CreateTopic("cap_info", sizeof(Cap_t));

  MsgDistrib_Subscriber_t* fb_sub = MsgDistrib_Subscribe("cap_fb", true);
  MsgDistrib_Subscriber_t* ref_sub =
      MsgDistrib_CreateTopic("referee_cap", true);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    MsgDistrib_Poll(ref_sub, &referee_cap, 0);

    /* 一定时间长度内接收不到电容反馈值，使电容离线 */
    if (MsgDistrib_Poll(fb_sub, &can_fb, 500) != pdPASS) {
      Cap_HandleOffline(&cap, &cap_out, GAME_CHASSIS_MAX_POWER_WO_REF);
    } else {
      vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      /* 根据裁判系统数据计算输出功率 */
      Cap_Update(&cap, &can_fb);
      Cap_Control(&referee_cap, &cap_out);
      xTaskResumeAll();
    }

    MsgDistrib_Publish(out_pub, &cap_out);
    MsgDistrib_Publish(ui_pub, &cap_ui);
    MsgDistrib_Publish(info_pub, &cap);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
