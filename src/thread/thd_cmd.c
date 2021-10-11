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

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_CMD(void* arg) {
  Runtime_t* runtime = arg;

  CMD_RC_t rc;
  CMD_Host_t host;
  CMD_t cmd;
  CMD_UI_t cmd_ui;

  MsgDist_Publisher_t* cmd_ai_pub =
      MsgDist_CreateTopic("cmd_ai", sizeof(Game_AI_Mode_t));
  MsgDist_Publisher_t* cmd_chassis_pub =
      MsgDist_CreateTopic("cmd_chassis", sizeof(CMD_ChassisCmd_t));
  MsgDist_Publisher_t* cmd_gimbal_pub =
      MsgDist_CreateTopic("cmd_gimbal", sizeof(CMD_GimbalCmd_t));
  MsgDist_Publisher_t* cmd_launcher_pub =
      MsgDist_CreateTopic("cmd_launcher", sizeof(CMD_LauncherCmd_t));
  MsgDist_Publisher_t* ui_cmd_pub =
      MsgDist_CreateTopic("ui_cmd", sizeof(CMD_UI_t));

  MsgDist_Subscriber_t* rc_sub = MsgDist_Subscribe("rc_cmd", true);
  MsgDist_Subscriber_t* host_sub = MsgDist_Subscribe("rc_host", true);

  /* 初始化指令处理 */
  CMD_Init(&cmd, &(runtime->cfg.pilot_cfg->param));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 将接收机数据解析为指令数据 */
    MsgDist_Poll(rc_sub, &rc, 0);  // TODO: 可以阻塞
    MsgDist_Poll(host_sub, &host, 0);

    CMD_ParseRc(&rc, &cmd, (float)THD_PERIOD_MS / 1000.0f);

    /* 判断是否需要让上位机覆写指令 */
    if (CMD_CheckHostOverwrite(&cmd)) {
      CMD_ParseHost(&host, &cmd, (float)THD_PERIOD_MS / 1000.0f);
    }
    CMD_PackUi(&cmd_ui, &cmd);

    MsgDist_Publish(cmd_ai_pub, &(cmd.ai_mode));
    MsgDist_Publish(cmd_chassis_pub, &(cmd.chassis));
    MsgDist_Publish(cmd_gimbal_pub, &(cmd.gimbal));
    MsgDist_Publish(cmd_launcher_pub, &(cmd.launcher));
    MsgDist_Publish(ui_cmd_pub, &cmd_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
