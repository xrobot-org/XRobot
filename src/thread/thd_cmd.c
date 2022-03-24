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
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_cmd(void* arg) {
  runtime_t* runtime = arg;

  cmd_rc_t rc;
  cmd_host_t host;
  cmd_t cmd;
  cmd_ui_t cmd_ui;

  om_topic_t* ch_tp = om_config_topic(NULL, "A", "cmd_chassis");
  om_topic_t* gm_tp = om_config_topic(NULL, "A", "cmd_gimbal");
  om_topic_t* la_tp = om_config_topic(NULL, "A", "cmd_launcher");
  om_topic_t* ui_tp = om_config_topic(NULL, "A", "cmd_ui");
  om_topic_t* rc_tp = om_find_topic("cmd_rc", UINT32_MAX);
  om_topic_t* host_tp = om_find_topic("cmd_host", UINT32_MAX);

  om_suber_t* rc_sub = om_subscript(rc_tp, OM_PRASE_VAR(rc), NULL);
  om_suber_t* host_sub = om_subscript(host_tp, OM_PRASE_VAR(host), NULL);

  /* 初始化指令处理 */
  cmd_init(&cmd, &(runtime->cfg.pilot_cfg->param),
           &(runtime->cfg.robot_param->default_mode));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 将接收机数据解析为指令数据 */
    om_suber_dump(rc_sub, false);  // TODO: 可以阻塞

    cmd_parse_rc(&rc, &cmd, (float)THD_PERIOD_MS / 1000.0f);

    /* 判断是否需要让上位机覆写指令 */
    if (cmd_check_host_overwrite(&rc, &cmd)) {
      if (om_suber_dump(host_sub, false) == OM_OK)
        cmd_parse_host(&host, &cmd, (float)THD_PERIOD_MS / 1000.0f);
    }
    cmd_pack_ui(&cmd_ui, &cmd);

    om_publish(ch_tp, OM_PRASE_VAR(cmd.chassis), true, false);
    om_publish(gm_tp, OM_PRASE_VAR(cmd.gimbal), true, false);
    om_publish(la_tp, OM_PRASE_VAR(cmd.launcher), true, false);
    om_publish(ui_tp, OM_PRASE_VAR(cmd_ui), true, false);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_cmd, 256, 3);
