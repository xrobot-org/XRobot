/**
 * @file atti_esti.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 姿态解算任务
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 控制IMU加热到指定温度防止温漂，收集IMU数据给AHRS算法。
 * 收集BMI088的数据，解算后得到四元数，转换为欧拉角之后放到消息队列中，等待其他任务取用。
 *
 */

/* Includes ----------------------------------------------------------------- */
#include <string.h>

#include "bsp/mm.h"
#include "bsp/pwm.h"
#include "bsp/usb.h"
#include "component/ahrs.h"
#include "component/pid.h"
#include "device/bmi088.h"
#include "device/ist8310.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
BMI088_t bmi088;
IST8310_t ist8310;

AHRS_t gimbal_ahrs;
AHRS_Eulr_t eulr_to_send;

KPID_t imu_temp_ctrl_pid;
#else
static BMI088_t bmi088;
static IST8310_t ist8310;

static AHRS_t gimbal_ahrs;
static AHRS_Eulr_t eulr_to_send;

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

/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 姿态解算
 *
 * \param argument 未使用
 */
void Task_AttiEsti(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 初始化设备 */
  BMI088_Init(&bmi088, &(task_runtime.cfg.cali.bmi088));
  // IST8310_Init(&ist8310, &(task_runtime.cfg.cali.ist8310));

  /* 读取一次磁力计数据，用以初始化姿态解算算法 */
  // IST8310_WaitNew(osWaitForever);
  // IST8310_StartDmaRecv();
  // IST8310_WaitDmaCplt();
  // IST8310_Parse(&ist8310);

  /* 初始化姿态解算算法 */
  AHRS_Init(&gimbal_ahrs, &ist8310.magn, BMI088_GetUpdateFreq(&bmi088));

  /* 初始化IMU温度控制PID，防止温漂 */
  PID_Init(&imu_temp_ctrl_pid, KPID_MODE_NO_D,
           1.0f / BMI088_GetUpdateFreq(&bmi088), &imu_temp_ctrl_pid_param);

  /* IMU温度控制PWM输出 */
  BSP_PWM_Start(BSP_PWM_IMU_HEAT);

  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.atti_esti =
        osThreadGetStackSpace(osThreadGetId());
#endif
    /* 等待IMU新数据 */
    BMI088_WaitNew();

    /* 开始数据接收DMA，加速度计和陀螺仪共用同一个SPI接口，
     * 一次只能开启一个DMA
     */
    BMI088_AcclStartDmaRecv();
    BMI088_AcclWaitDmaCplt();

    BMI088_GyroStartDmaRecv();
    BMI088_GyroWaitDmaCplt();

    /* 磁力计的数据接收频率远小于IMU，
     * 这里使用非阻塞操作，保证姿态解算实时性
     */
    // IST8310_WaitNew(0);
    // IST8310_StartDmaRecv();
    // IST8310_WaitDmaCplt();

    /* 锁住RTOS内核防止数据解析过程中断，造成错误 */
    osKernelLock();
    /* 接收完所有数据后，把数据从原始字节加工成方便计算的数据 */
    BMI088_ParseAccl(&bmi088);
    BMI088_ParseGyro(&bmi088);
    // IST8310_Parse(&ist8310);

    /* 根据设备接收到的数据进行姿态解析 */
    AHRS_Update(&gimbal_ahrs, &bmi088.accl, &bmi088.gyro, &ist8310.magn);

    /* 根据解析出来的四元数计算欧拉角 */
    AHRS_GetEulr(&eulr_to_send, &gimbal_ahrs);
    osKernelUnlock();

    /* 将需要与其他任务分享的数据放到消息队列中 */
    osMessageQueueReset(task_runtime.msgq.gimbal.accl);
    osMessageQueuePut(task_runtime.msgq.gimbal.accl, &bmi088.accl, 0, 0);
    osMessageQueueReset(task_runtime.msgq.gimbal.eulr_imu);
    osMessageQueuePut(task_runtime.msgq.gimbal.eulr_imu, &eulr_to_send, 0, 0);
    osMessageQueueReset(task_runtime.msgq.ai.quat);
    osMessageQueuePut(task_runtime.msgq.ai.quat, &(gimbal_ahrs.quat), 0, 0);
    osMessageQueueReset(task_runtime.msgq.gimbal.gyro);
    osMessageQueuePut(task_runtime.msgq.gimbal.gyro, &bmi088.gyro, 0, 0);

    /* PID控制IMU温度，PWM输出 */
    BSP_PWM_Set(BSP_PWM_IMU_HEAT,
                PID_Calc(&imu_temp_ctrl_pid, 40.0f, bmi088.temp, 0.0f, 0.0f));
  }
}
