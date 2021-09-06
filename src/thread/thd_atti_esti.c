/**
 * @file thd_atti_esti.c
 * @author Qu Shen
 * @brief 姿态解算线程
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 * 收集传感器数据，解算后得到四元数，转换为欧拉角之后发布。
 */

#include <string.h>

#include "comp_ahrs.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

AHRS_t gimbal_ahrs;
AHRS_Eulr_t gimbal_eulr;
Vector3_t accl, gyro;

#else

static AHRS_t gimbal_ahrs;

#endif

/**
 * @brief 姿态解算
 *
 * @param argument 未使用
 */
void Thread_AttiEsti(void* argument) {
  RM_UNUSED(argument);

  MsgDistrib_Publisher_t* gimbal_eulr_pub =
      MsgDistrib_CreateTopic("gimbal_eulr", sizeof(AHRS_Eulr_t));

  MsgDistrib_Subscriber_t* accl_sub = MsgDistrib_Subscribe("gimbal_accl", true);
  MsgDistrib_Subscriber_t* gyro_sub = MsgDistrib_Subscribe("gimbal_gyro", true);

  /* 初始化姿态解算算法 */
  AHRS_Init(&gimbal_ahrs, NULL, TASK_FREQ_ATTI_ESTI);

  while (1) {
    MsgDistrib_Poll(accl_sub, &accl, 0);
    MsgDistrib_Poll(gyro_sub, &gyro, 0);

    /* 锁住RTOS内核防止数据解析过程中断，造成错误 */
    vTaskSuspendAll();

    /* 根据设备接收到的数据进行姿态解析 */
    AHRS_Update(&gimbal_ahrs, &accl, &gyro, NULL);

    /* 根据解析出来的四元数计算欧拉角 */
    AHRS_GetEulr(&gimbal_eulr, &gimbal_ahrs);
    xTaskResumeAll();

    /* 发布数据 */
    MsgDistrib_Publish(gimbal_eulr_pub, &gimbal_eulr);
  }
}
