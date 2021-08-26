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
 * 或者从上位机中接收数据。
 * 根据条件计算最终命令CMD_t。把计算好的CMD_t细分后放到对应的消息队列中。
 * 超时未收到则认为是丢控等特殊情况，把CMD_RC_t中的内容置零，
 * 在后续的CMD_ParseRc中会根据此内容发现错误，保证机器人不失控。
 *
 */

/* Includes ----------------------------------------------------------------- */
#include <string.h>

#include "dev_dr16.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
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

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 控制指令接收
 *
 * @param argument 未使用
 */
void Thread_CMD(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CTRL_COMMAND);

  /* 初始化指令处理 */
  CMD_Init(&cmd, &(runtime.cfg.pilot_cfg->param));

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 将接收机数据解析为指令数据 */
    if (xQueueReceive(runtime.msgq.cmd.src.rc, &rc, 0) == pdPASS)
      CMD_ParseRc(&rc, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);

    /* 判断是否需要让上位机覆写指令 */
    if (CMD_CheckHostOverwrite(&cmd)) {
      if (xQueueReceive(runtime.msgq.cmd.src.host, &host, 0) == pdPASS) {
        CMD_ParseHost(&host, &cmd, 1.0f / (float)TASK_FREQ_CTRL_COMMAND);
      }
    }
    CMD_PackUi(&cmd_ui, &cmd);

    /* 将需要与其他任务分享的数据放到消息队列中 */
    xQueueOverwrite(runtime.msgq.cmd.ai, &(cmd.ai_mode));

    xQueueOverwrite(runtime.msgq.cmd.chassis, &(cmd.chassis));

    xQueueOverwrite(runtime.msgq.cmd.gimbal, &(cmd.gimbal));

    xQueueOverwrite(runtime.msgq.cmd.launcher, &(cmd.launcher));

    xQueueOverwrite(runtime.msgq.ui.cmd, &cmd_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
