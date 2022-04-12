/*
  AI上位机通信任务
*/

#include "dev_ai.h"
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ai(void* arg) {
  RM_UNUSED(arg);

  ai_t ai;
  cmd_host_t cmd_host;
  quaternion_t ai_quat;
  referee_for_ai_t referee_ai;
  eulr_t ai_eulr;

  om_topic_t* host_tp = om_config_topic(NULL, "A", "cmd_host");
  om_topic_t* eulr = om_find_topic("gimbal_eulr", UINT32_MAX);
  om_topic_t* quat_tp = om_find_topic("gimbal_quat", UINT32_MAX);
  om_topic_t* ref_tp = om_find_topic("referee_ai", UINT32_MAX);

  om_suber_t* quat_sub = om_subscript(quat_tp, OM_PRASE_VAR(ai_quat));
  om_suber_t* ref_sub = om_subscript(ref_tp, OM_PRASE_VAR(referee_ai));
  om_suber_t* eulr_sub = om_subscript(eulr, OM_PRASE_VAR(ai_eulr));

#if HOST_USB_DISABLE
  vTaskSuspend(xTaskGetCurrentTaskHandle());
#endif

  /* 初始化AI通信 */
  ai_init(&ai);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 接收指令 */
    if (ai_wait_recv_cplt(&ai)) {
      ai_parse_host(&ai, xTaskGetTickCount());
    } else {
      ai_handle_offline(&ai, xTaskGetTickCount());
    }

    /* AI在线,发布控制命令 */
    if (ai.online) {
      om_suber_export(eulr_sub, false);
      ai_pack_cmd(&ai, &cmd_host, &ai_eulr);
      om_publish(host_tp, OM_PRASE_VAR(cmd_host), true, false);
    }

    /* 发送数据到上位机 */
    om_suber_export(quat_sub, false);
    ai_pack_mcu_for_host(&ai, &ai_quat);

    if (om_suber_export(ref_sub, false) == OM_OK) {
      ai_pack_ref_for_host(&ai, &(referee_ai));
    }

    ai_start_trans(&ai);

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ai, 256, 4);
