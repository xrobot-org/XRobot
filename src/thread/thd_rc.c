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
#include "mid_msg_dist.h"
#include "thd.h"

void thd_rc(void* arg) {
  RM_UNUSED(arg);

  dr16_t dr16;
  cmd_rc_t cmd_rc;

  publisher_t* rc_pub = msg_dist_create_topic("cmd_rc", sizeof(cmd_rc_t));

  dr16_init(&dr16); /* 初始化dr16 */

  while (1) {
    /* 开启DMA */
    dr16_start_dma_recv(&dr16);

    /* 等待DMA完成 */
    if (dr16_wait_dma_cplt(20)) {
      /* 进行解析 */
      dr16_parse_rc(&dr16, &cmd_rc);
    } else {
      /* 处理遥控器离线 */
      dr16_handle_offline(&dr16, &cmd_rc);
    }

    msg_dist_publish(rc_pub, &cmd_rc);
  }
}
THREAD_DECLEAR(thd_rc, 128, 4);
