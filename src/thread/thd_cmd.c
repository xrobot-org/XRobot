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
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_cmd(void* arg) {
  runtime_t* runtime = arg;

  cmd_rc_t rc;
  cmd_host_t host;
  cmd_t cmd;
  cmd_ui_t cmd_ui;

  publisher_t* cmd_ai_pub = msg_dist_create_topic("cmd_ai", sizeof(ai_mode_t));
  publisher_t* cmd_chassis_pub =
      msg_dist_create_topic("cmd_chassis", sizeof(cmd_chassis_t));
  publisher_t* cmd_gimbal_pub =
      msg_dist_create_topic("cmd_gimbal", sizeof(cmd_gimbal_t));
  publisher_t* cmd_launcher_pub =
      msg_dist_create_topic("cmd_launcher", sizeof(cmd_launcher_t));
  publisher_t* ui_cmd_pub = msg_dist_create_topic("ui_cmd", sizeof(cmd_ui_t));

  subscriber_t* rc_sub = msg_dist_subscribe("cmd_rc", true);
  subscriber_t* host_sub = msg_dist_subscribe("cmd_host", true);

  /* 初始化指令处理 */
  cmd_init(&cmd, &(runtime->cfg.pilot_cfg->param));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 将接收机数据解析为指令数据 */
    msg_dist_poll(rc_sub, &rc, 0);  // TODO: 可以阻塞
    msg_dist_poll(host_sub, &host, 0);

    cmd_parse_rc(&rc, &cmd, (float)THD_PERIOD_MS / 1000.0f);

    /* 判断是否需要让上位机覆写指令 */
    if (cmd_check_host_overwrite(&cmd)) {
      cmd_parse_host(&host, &cmd, (float)THD_PERIOD_MS / 1000.0f);
    }
    cmd_pack_ui(&cmd_ui, &cmd);

    msg_dist_publish(cmd_ai_pub, &(cmd.ai_mode));
    msg_dist_publish(cmd_chassis_pub, &(cmd.chassis));
    msg_dist_publish(cmd_gimbal_pub, &(cmd.gimbal));
    msg_dist_publish(cmd_launcher_pub, &(cmd.launcher));
    msg_dist_publish(ui_cmd_pub, &cmd_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_cmd, 128, 3);
