/*
  AI上位机通信任务
*/

#include "dev_ai.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ai(void* arg) {
  RM_UNUSED(arg);

  ai_t ai;
  ai_ui_t ai_ui;
  cmd_host_t cmd_host;
  quaternion_t ai_quat;
  referee_for_ai_t referee_ai;

  publisher_t* cmd_host_pub =
      msg_dist_create_topic("cmd_host", sizeof(cmd_launcher_t));
  publisher_t* ui_ai_pub = msg_dist_create_topic("ui_ai", sizeof(cmd_ui_t));

  subscriber_t* quat_sub = msg_dist_subscribe("gimbal_quat", true);
  subscriber_t* cmd_ai_sub = msg_dist_subscribe("cmd_ai", true);
  subscriber_t* referee_ai_sub = msg_dist_subscribe("referee_ai", true);

  /* 初始化AI通信 */
  ai_init(&ai);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 接收指令 */
    ai_start_receiving(&ai);

    if (ai_wait_recv_cplt(&ai, THD_PERIOD_MS)) {
      ai_parse_host(&ai);
    } else {
      ai_handle_offline(&ai);
    }

    ai_pack_cmd(&ai, &cmd_host);
    msg_dist_publish(cmd_host_pub, &cmd_host);

    /* 发送数据 */
    msg_dist_poll(cmd_ai_sub, &(ai.mode), 0);
    msg_dist_poll(quat_sub, &ai_quat, 0);
    ai_pack_mcu_for_host(&ai, &ai_quat);

    if (msg_dist_poll(referee_ai_sub, &(referee_ai), 0)) {
      ai_pack_ref_for_host(&ai, &(referee_ai));
    }

    if (ai_wait_trans_cplt(&ai, 0)) {
      ai_start_trans(&ai);
    }

    /* 更新UI */
    ai_pack_ui(&ai_ui, &ai);
    msg_dist_publish(ui_ai_pub, &cmd_host);

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ai, 128, 4);
