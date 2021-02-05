/*
  底盘控制任务

  控制底盘行为。

  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "component\limiter.h"
#include "module\chassis.h"
#include "module\config.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;
float power_lim = 40.f; /* 最大输出功率 */
#ifdef DEBUG
CMD_ChassisCmd_t chassis_cmd;
Chassis_t chassis;
CAN_ChassisOutput_t chassis_out;
CAN_Capacitor_t cap;
Referee_t ref_chassis;
#else
static CMD_ChassisCmd_t chassis_cmd;
static Chassis_t chassis;
static CAN_ChassisOutput_t chassis_out;
static CAN_Capacitor_t cap;
static Referee_t ref_chassis;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制底盘
 *
 * \param argument 未使用
 */
void Task_CtrlChassis(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_CHASSIS;

  /* 初始化底盘 */
  Chassis_Init(&chassis, &(task_runtime.cfg.robot_param->chassis),
               &task_runtime.cfg.mech_zero, (float)TASK_FREQ_CTRL_CHASSIS);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.chassis, &can.motor.chassis,
                    NULL, osWaitForever);

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  uint32_t wakeup = HAL_GetTick(); /* 计算任务运行间隔的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_chassis = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    /* 读取CAN总线电机指令、控制指令、电容反馈*/
    osMessageQueueGet(task_runtime.msgq.referee.chassis, &ref_chassis, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.can.feedback.chassis, &can, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cmd.chassis, &chassis_cmd, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cap_info, &cap, NULL, 0);
    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    const uint32_t now = HAL_GetTick();
    Chassis_UpdateFeedback(&chassis, &can);
    Chassis_Control(&chassis, &chassis_cmd, (float)(now - wakeup) / 1000.0f);
    Chassis_PowerLimit(&chassis, &cap, &ref_chassis);
    Chassis_DumpOutput(&chassis, &chassis_out);
    wakeup = now;
    osKernelUnlock();

    osMessageQueuePut(task_runtime.msgq.can.output.chassis, &chassis_out, 0, 0);
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
