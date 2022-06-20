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
#include "comp_pid.hpp"
#include "dev_bmi088.hpp"
#include "om.h"
#include "thd.hpp"

static const kpid_params_t imu_temp_ctrl_pid_param = {
    .k = 0.15f,
    .p = 1.0f,
    .i = 0.0f,
    .d = 0.0f,
    .i_limit = 1.0f,
    .out_limit = 1.0f,
};

void thd_imu(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;

  bmi088_t bmi088;
  kpid_t imu_temp_ctrl_pid;

  om_topic_t* accl_pub = om_config_topic(NULL, "A", "gimbal_accl");
  om_topic_t* gyro_pub = om_config_topic(NULL, "A", "gimbal_gyro");

  /* 初始化设备 */
  bmi088_init(&bmi088, &(runtime->cfg.cali.bmi088),
              &(runtime->cfg.robot_param->imu));

  /* 初始化IMU温度控制PID，防止温漂 */
  kpid_init(&imu_temp_ctrl_pid, KPID_MODE_NO_D, KPID_MODE_K_SET,
            1.0f / bmi088_get_update_freq(&bmi088), &imu_temp_ctrl_pid_param);

  /* IMU温度控制PWM输出 */
  bsp_pwm_start(BSP_PWM_IMU_HEAT);

  while (1) {
    /* 等待IMU新数据 */
    if (bmi088_wait_new(&bmi088, 20)) {
      if (bmi088_accl_wait_new(&bmi088, 0)) {
        /* 开始数据接收DMA，加速度计和陀螺仪共用同一个SPI接口，
         * 一次只能开启一个DMA
         */
        bmi088_accl_start_dma_recv();
        bmi088_accl_wait_dma_cplt(&bmi088);
        bmi088_parse_accl(&bmi088);

        om_publish(accl_pub, OM_PRASE_VAR(bmi088.accl), true, false);
      }
      if (bmi088_gyro_wait_new(&bmi088, 0)) {
        bmi088_gyro_start_dma_recv();
        bmi088_gyro_wait_dma_cplt(&bmi088);
        bmi088_parse_gyro(&bmi088);

        om_publish(gyro_pub, OM_PRASE_VAR(bmi088.gyro), true, false);
      }
    }
    // TODO: 添加滤波

    /* PID控制IMU温度，PWM输出 */
    bsp_pwm_set_comp(BSP_PWM_IMU_HEAT, kpid_calc(&imu_temp_ctrl_pid, 40.0f,
                                                 bmi088.temp, 0.0f, 0.0f));
  }
}
THREAD_DECLEAR(thd_imu, 256, 4);
