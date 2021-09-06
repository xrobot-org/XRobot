/**
 * @file thd_imu.c
 * @author Qu Shen
 * @brief IMU数据采集
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 * 控制IMU加热到指定温度防止温漂
 * 采集IMU数据，对其进行滤波等处理
 */

#include <string.h>

#include "bsp_pwm.h"
#include "comp_pid.h"
#include "dev_bmi088.h"
#include "mid_msg_distrib.h"
#include "thd.h"

#ifdef MCU_DEBUG_BUILD

BMI088_t bmi088;
KPID_t imu_temp_ctrl_pid;

#else

static BMI088_t bmi088;
static KPID_t imu_temp_ctrl_pid;

#endif

static const KPID_Params_t imu_temp_ctrl_pid_param = {
    .k = 0.15f,
    .p = 1.0f,
    .i = 0.0f,
    .d = 0.0f,
    .i_limit = 1.0f,
    .out_limit = 1.0f,
};

/**
 * @brief IMU数据采集
 *
 * @param argument runtime
 */
void Thread_AttiEsti(void* argument) {
  Runtime_t* runtime = argument;

  MsgDistrib_Publisher_t* accl_pub =
      MsgDistrib_CreateTopic("gimbal_accl", sizeof(Vector3_t));
  MsgDistrib_Publisher_t* gyro_pub =
      MsgDistrib_CreateTopic("gimbal_gyro", sizeof(Vector3_t));

  /* 初始化设备 */
  BMI088_Init(&bmi088, &(runtime->cfg.cali.bmi088));

  /* 初始化IMU温度控制PID，防止温漂 */
  PID_Init(&imu_temp_ctrl_pid, KPID_MODE_NO_D,
           1.0f / BMI088_GetUpdateFreq(&bmi088), &imu_temp_ctrl_pid_param);

  /* IMU温度控制PWM输出 */
  BSP_PWM_Start(BSP_PWM_IMU_HEAT);

  while (1) {
    /* 等待IMU新数据 */
    BMI088_WaitNew();

    /* 开始数据接收DMA，加速度计和陀螺仪共用同一个SPI接口，
     * 一次只能开启一个DMA
     */
    BMI088_AcclStartDmaRecv();
    BMI088_AcclWaitDmaCplt();

    BMI088_GyroStartDmaRecv();
    BMI088_GyroWaitDmaCplt();

    /* 锁住RTOS内核防止数据解析过程中断，造成错误 */
    vTaskSuspendAll();
    /* 接收完所有数据后，把数据从原始字节加工成方便计算的数据 */
    BMI088_ParseAccl(&bmi088);
    BMI088_ParseGyro(&bmi088);
    xTaskResumeAll();

    // TODO: 添加滤波

    /* 发布消息 */
    MsgDistrib_Publish(accl_pub, &bmi088.accl);
    MsgDistrib_Publish(gyro_pub, &bmi088.gyro);

    /* PID控制IMU温度，PWM输出 */
    BSP_PWM_Set(BSP_PWM_IMU_HEAT,
                PID_Calc(&imu_temp_ctrl_pid, 40.0f, bmi088.temp, 0.0f, 0.0f));
  }
}
