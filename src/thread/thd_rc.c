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
#include "om.h"
#include "thd.h"

void thd_rc(void* arg) {
  RM_UNUSED(arg);

  dr16_t dr16;
  cmd_rc_t cmd_rc;

  om_topic_t* rc_pub = om_config_topic(NULL, "A", "cmd_rc");

  dr16_init(&dr16); /* 初始化dr16 */

  while (1) {
    /* 开启DMA */
    dr16_start_dma_recv(&dr16);

    /* 等待DMA完成 */
    if (dr16_wait_dma_cplt(&dr16, 20)) {
      /* 进行解析 */
      dr16_parse_rc(&dr16, &cmd_rc);
    } else {
      /* 处理遥控器离线 */
      dr16_handle_offline(&dr16, &cmd_rc);
    }

    om_publish(rc_pub, OM_PRASE_VAR(cmd_rc), true, false);
  }
}
THREAD_DECLEAR(thd_rc, 128, 4);
