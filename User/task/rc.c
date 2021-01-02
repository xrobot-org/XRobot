/*
  DR16接收机通信任务
*/

/* Includes ----------------------------------------------------------------- */
#include <string.h>

#include "device\dr16.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef DEBUG
DR16_t dr16;
CMD_RC_t rc;
#else
static DR16_t dr16;
static CMD_RC_t rc;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief dr16接收机
 *
 * \param argument 未使用
 */
void Task_RC(void *argument) {
  (void)argument; /* 未使用，消除警告 */

  DR16_Init(&dr16); /* 初始化dr16 */

  while (1) {
#ifdef DEBUG
    /*  */
    task_runtime.stack_water_mark.command = osThreadGetStackSpace(NULL);
#endif
    /* 开启DMA */
    DR16_StartDmaRecv(&dr16);

    if (DR16_WaitDmaCplt(20)) {
      /* 转换 */
      DR16_ParseRC(&dr16, &rc);
    } else {
      /* 处理遥控器离线 */
      DR16_HandleOffline(&dr16, &rc);
    }
    osMessageQueuePut(task_runtime.msgq.raw_cmd.rc_raw, &rc, 0, 0);
  }
}
