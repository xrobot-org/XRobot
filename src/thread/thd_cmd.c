/**
 * @file cmd.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 接收机器人的控制指令
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 从DR16中接收数据，转换为通用的CMD_RC_t控制信号。
 * 从上位机中接收数据。
 * 根据条件计算最终命令CMD_t。分开发布。
 *
 */

#include <string.h>

#include "dev_dr16.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD
CMD_RC_t rc;
CMD_Host_t host;
CMD_t cmd;
CMD_UI_t cmd_ui;
#else
static CMD_RC_t rc;
static CMD_Host_t host;
static CMD_t cmd;
static CMD_UI_t cmd_ui;
#endif

/**
 * @brief 控制指令接收
 *
 * @param argument 未使用
 */
void Thread_CMD(void* argument) {
  Runtime_t* runtime = argument;
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CTRL_COMMAND);

  MsgDistrib_Publisher_t* cmd_ai_pub =
      MsgDistrib_CreateTopic("cmd_ai", sizeof(Game_AI_Mode_t));
  MsgDistrib_Publisher_t* cmd_chassis_pub =
      MsgDistrib_CreateTopic("cmd_chassis", sizeof(CMD_ChassisCmd_t));
  MsgDistrib_Publisher_t* cmd_gimbal_pub =
      MsgDistrib_CreateTopic("cmd_gimbal", sizeof(CMD_GimbalCmd_t));
  MsgDistrib_Publisher_t* cmd_launcher_pub =
      MsgDistrib_CreateTopic("cmd_launcher", sizeof(CMD_LauncherCmd_t));
  MsgDistrib_Publisher_t* ui_cmd_pub =
      MsgDistrib_CreateTopic("ui_cmd", sizeof(CMD_UI_t));

  MsgDistrib_Subscriber_t* rc_sub = MsgDistrib_CreateTopic("rc_cmd", true);
  MsgDistrib_Subscriber_t* host_sub = MsgDistrib_Subscribe("rc_host", true);

  /* 初始化指令处理 */
  CMD_Init(&cmd, &(runtime->cfg.pilot_cfg->param));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 将接收机数据解析为指令数据 */
    MsgDistrib_Poll(rc_sub, &rc, 0);  // TODO: 可以阻塞
    MsgDistrib_Poll(host_sub, &host, 0);

    CMD_ParseRc(&rc, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);

    /* 判断是否需要让上位机覆写指令 */
    if (CMD_CheckHostOverwrite(&cmd)) {
      CMD_ParseHost(&host, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);
    }
    CMD_PackUi(&cmd_ui, &cmd);

    MsgDistrib_Publish(cmd_ai_pub, &(cmd.ai_mode));
    MsgDistrib_Publish(cmd_chassis_pub, &(cmd.chassis));
    MsgDistrib_Publish(cmd_gimbal_pub, &(cmd.gimbal));
    MsgDistrib_Publish(cmd_launcher_pub, &(cmd.launcher));
    MsgDistrib_Publish(ui_cmd_pub, &cmd_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
