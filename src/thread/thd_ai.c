/*
  AI上位机通信任务
*/

#include "dev_ai.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_AI(void* arg) {
  RM_UNUSED(arg);

  AI_t ai;
  AI_UI_t ai_ui;
  CMD_Host_t cmd_host;
  AHRS_Quaternion_t ai_quat;
  Referee_ForAI_t referee_ai;

  MsgDist_Publisher_t* cmd_host_pub =
      MsgDist_CreateTopic("cmd_host", sizeof(CMD_LauncherCmd_t));
  MsgDist_Publisher_t* ui_ai_pub =
      MsgDist_CreateTopic("ui_ai", sizeof(CMD_UI_t));

  MsgDist_Subscriber_t* quat_sub = MsgDist_Subscribe("gimbal_quat", true);
  MsgDist_Subscriber_t* cmd_ai_sub = MsgDist_Subscribe("cmd_ai", true);
  MsgDist_Subscriber_t* referee_ai_sub = MsgDist_Subscribe("referee_ai", true);

  /* 初始化AI通信 */
  AI_Init(&ai);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 接收指令 */
    AI_StartReceiving(&ai);

    if (AI_WaitRecvCplt(&ai, THD_PERIOD_MS)) {
      AI_ParseHost(&ai);
    } else {
      AI_HandleOffline(&ai);
    }

    AI_PackCMD(&ai, &cmd_host);
    MsgDist_Publish(cmd_host_pub, &cmd_host);

    /* 发送数据 */
    MsgDist_Poll(cmd_ai_sub, &(ai.mode), 0);
    MsgDist_Poll(quat_sub, &ai_quat, 0);
    AI_PackMcuForHost(&ai, &ai_quat);

    if (MsgDist_Poll(referee_ai_sub, &(referee_ai), 0)) {
      AI_PackRefForHost(&ai, &(referee_ai));
    }

    if (AI_WaitTransCplt(&ai, 0)) {
      AI_StartTrans(&ai);
    }

    /* 更新UI */
    AI_PackUI(&ai_ui, &ai);
    MsgDist_Publish(ui_ai_pub, &cmd_host);

    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
