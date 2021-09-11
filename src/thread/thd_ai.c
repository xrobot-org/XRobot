/*
  AI上位机通信任务
*/

#include "dev_ai.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

AI_t ai;
AI_UI_t ai_ui;
CMD_Host_t cmd_host;
AHRS_Quaternion_t ai_quat;
Referee_ForAI_t referee_ai;

#else

static AI_t ai;
static AI_UI_t ai_ui;
static CMD_Host_t cmd_host;
static AHRS_Quaternion_t ai_quat;
static Referee_ForAI_t referee_ai;

#endif

#define THD_PERIOD_MS (2)

void Thd_AI(void* argument) {
  RM_UNUSED(argument);
  const uint32_t delay_tick = pdMS_TO_TICKS(THD_PERIOD_MS);

  MsgDistrib_Publisher_t* cmd_host_pub =
      MsgDistrib_CreateTopic("cmd_host", sizeof(CMD_LauncherCmd_t));
  MsgDistrib_Publisher_t* ui_ai_pub =
      MsgDistrib_CreateTopic("ui_ai", sizeof(CMD_UI_t));

  MsgDistrib_Subscriber_t* quat_sub = MsgDistrib_Subscribe("gimbal_quat", true);
  MsgDistrib_Subscriber_t* cmd_ai_sub = MsgDistrib_Subscribe("cmd_ai", true);
  MsgDistrib_Subscriber_t* referee_ai_sub =
      MsgDistrib_Subscribe("referee_ai", true);

  /* 初始化AI通信 */
  AI_Init(&ai);

  uint32_t previous_wake_time = xTaskGetTickCount();
  uint32_t missed_delay = 0;

  while (1) {
    MsgDistrib_Poll(quat_sub, &ai_quat, 0);
    MsgDistrib_Poll(cmd_ai_sub, &(ai.mode), 0);

    bool ref_update =
        (MsgDistrib_Poll(referee_ai_sub, &(referee_ai), 0) == pdPASS);

    AI_StartReceiving(&ai);
    // TODO: wait里面必须加timeout
    if (AI_WaitDmaCplt()) {
      AI_ParseHost(&ai);
    } else {
      if (missed_delay > 300) AI_HandleOffline(&ai);
    }

    if (ai.mode != AI_MODE_STOP && ai.ai_online) {
      AI_PackCmd(&ai, &cmd_host);
      MsgDistrib_Publish(cmd_host_pub, &cmd_host);
    }

    AI_PackMcu(&ai, &ai_quat);
    if (ref_update) AI_PackRef(&ai, &(referee_ai));

    AI_StartTrans(&ai, ref_update);

    AI_PackUi(&ai_ui, &ai);
    MsgDistrib_Publish(ui_ai_pub, &cmd_host);
    missed_delay += xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
