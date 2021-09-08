/**
 * @file rc.c
 * @author Qu Shen (503578404@qq.com)
 * @brief DR16接收机通信线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自DR16的数据
 * 解析为通用的控制数据后发布
 *
 */

#include <string.h>

#include "dev_dr16.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

DR16_t dr16;
CMD_RC_t cmd_rc;

#else

static DR16_t dr16;
static CMD_RC_t cmd_rc;

#endif

#define THD_PERIOD_MS (2)

void Thd_RC(void* argument) {
  Runtime_t* runtime = argument;

  MsgDistrib_Publisher_t* rc_pub =
      MsgDistrib_CreateTopic("rc_cmd", sizeof(CMD_RC_t));

  DR16_Init(&dr16); /* 初始化dr16 */

  while (1) {
    /* 开启DMA */
    DR16_StartDmaRecv(&dr16);

    /* 等待DMA完成 */
    if (DR16_WaitDmaCplt(20)) {
      /* 进行解析 */
      DR16_ParseRC(&dr16, &cmd_rc);
    } else {
      /* 处理遥控器离线 */
      DR16_HandleOffline(&dr16, &cmd_rc);
    }

    MsgDistrib_Publish(rc_pub, &cmd_rc);
  }
}
