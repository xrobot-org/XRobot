/**
 * @file rc.c
 * @author Qu Shen (503578404@qq.com)
 * @brief DR16接收机通信任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自DR16的数据
 * 解析为通用的控制数据
 * 放入消息队列供其他任务使用
 *
 */

/* Includes ----------------------------------------------------------------- */

#include <string.h>

#include "device/dr16.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
DR16_t dr16;
CMD_RC_t cmd_rc;
#else
static DR16_t dr16;
static CMD_RC_t cmd_rc;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief dr16接收机
 *
 * \param argument 未使用
 */
void Task_RC(void *argument) {
  UNUSED(argument); /* 未使用，消除警告 */

  DR16_Init(&dr16); /* 初始化dr16 */

  while (1) {
#ifdef DEBUG
    /*  */
    task_runtime.stack_water_mark.rc = osThreadGetStackSpace(osThreadGetId());
#endif
    /* 开启DMA */
    DR16_StartDmaRecv(&dr16);

    /* 等待DMA完成 */
    if (DR16_WaitDmaCplt(20)) {
      /* 预计时间内收到后进行解析 */
      DR16_ParseRC(&dr16, &cmd_rc);
    } else {
      /* 处理遥控器离线 */
      DR16_HandleOffline(&dr16, &cmd_rc);
    }
    /* 发送给cmd任务，进行处理 */
    osMessageQueueReset(task_runtime.msgq.cmd.raw.rc);
    osMessageQueuePut(task_runtime.msgq.cmd.raw.rc, &cmd_rc, 0, 0);
  }
}
