/**
 * @file referee.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 裁判系统接收发送线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自裁判系统的数据
 * 解析后根据需求组合成新包发给各个模块
 * 无论裁判系统是否在线，都要按时发送给各个模块
 *
 */

#include "dev_referee.h"
#include "mid_msg_dist.h"
#include "thd.h"

#define THD_PERIOD_MS (1)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ref_recv(void* arg) {
  RM_UNUSED(arg);

  referee_recv_t ref;
  referee_for_ai_t for_ai;
  referee_for_chassis_t for_chassis;
  referee_for_launcher_t for_launcher;

  publisher_t* referee_ai_pub =
      msg_dist_create_topic("referee_ai", sizeof(referee_for_ai_t));
  publisher_t* referee_chassis_pub =
      msg_dist_create_topic("referee_chassis", sizeof(referee_for_chassis_t));
  publisher_t* referee_launcher_pub =
      msg_dist_create_topic("referee_launcher", sizeof(referee_for_launcher_t));

  /* 初始化裁判系统 */
  referee_recv_init(&ref);

  while (1) {
    referee_start_receiving(&ref); /* 开始接收裁判系统数据 */

#if REF_FORCE_ONLINE
    referee_wait_recv_cplt(&ref, 100);
    referee_parse(&ref);
#else
    if (!referee_wait_recv_cplt(&ref, 100)) { /* 判断裁判系统数据是否接收完成 */
      referee_handle_offline(&ref); /* 长时间未接收到数据，裁判系统离线 */
    } else {
      referee_parse(&ref); /* 解析裁判系统数据 */
    }
#endif

    /* 打包裁判系统数据 */
    referee_pack_for_ai(&for_ai, &ref);
    referee_pack_for_launcher(&for_launcher, &ref);
    referee_pack_for_chassis(&for_chassis, &ref);

    /* 发送裁判系统数据到其他进程 */
    msg_dist_publish(referee_ai_pub, &for_ai);
    msg_dist_publish(referee_chassis_pub, &for_chassis);
    msg_dist_publish(referee_launcher_pub, &for_launcher);
  }
}
THREAD_DECLEAR(thd_ref_recv, 512, 4);
