/**
 * @file ctrl_chassis.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘控制任务
 * @version 1.0.0
 * @date 2021-04-14
 *
 * @copyright Copyright (c) 2021
 *
 * 通过消息队列收集底盘控制需要的数据
 * 运行chassis模组
 * 通过消息队列发送底盘控制输出的数据
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "component/limiter.h"
#include "module/chassis.h"
#include "module/config.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

static CAN_t can;
static Cap_t cap;

#ifdef DEBUG
CMD_ChassisCmd_t chassis_cmd;
Chassis_t chassis;
CAN_ChassisOutput_t chassis_out;
Referee_ForChassis_t referee_chassis;
UI_ChassisUI_t chassis_ui;
#else
static CMD_ChassisCmd_t chassis_cmd;
static Chassis_t chassis;
static CAN_ChassisOutput_t chassis_out;
static Referee_ForChassis_t referee_chassis;
static UI_ChassisUI_t chassis_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 控制底盘
 *
 * \param argument 未使用
 */
void Task_CtrlChassis(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_CHASSIS;
  /* 初始化底盘 */
  Chassis_Init(&chassis, &(task_runtime.cfg.robot_param->chassis),
               &task_runtime.cfg.mech_zero, (float)TASK_FREQ_CTRL_CHASSIS);

  /* 延时一段时间再开启任务 */
  osMessageQueueGet(task_runtime.msgq.can.feedback.chassis, &can, NULL,
                    osWaitForever);
  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.ctrl_chassis =
        osThreadGetStackSpace(osThreadGetId());
#endif

    osMessageQueueGet(task_runtime.msgq.can.feedback.chassis, &can, NULL, 0);
    /* 读取控制指令、电容反馈、裁判系统 */
    osMessageQueueGet(task_runtime.msgq.referee.chassis, &referee_chassis, NULL,
                      0);
    osMessageQueueGet(task_runtime.msgq.cmd.chassis, &chassis_cmd, NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cap_info, &cap, NULL, 0);

    osKernelLock(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    Chassis_UpdateFeedback(&chassis, &can); /* 更新反馈值 */
    /* 根据遥控器命令计算底盘输出 */
    Chassis_Control(&chassis, &chassis_cmd, tick);
    Chassis_PowerLimit(&chassis, &cap, &referee_chassis); /* 限制输出功率 */
    Chassis_PackOutput(&chassis, &chassis_out);
    Chassis_PackUi(&chassis, &chassis_ui);
    osKernelUnlock();

    /* 将电机输出值发送到CAN */
    osMessageQueueReset(task_runtime.msgq.can.output.chassis);
    osMessageQueuePut(task_runtime.msgq.can.output.chassis, &chassis_out, 0, 0);

    /* 将底盘数据发送给UI */
    osMessageQueueReset(task_runtime.msgq.ui.chassis);
    osMessageQueuePut(task_runtime.msgq.ui.chassis, &chassis_ui, 0, 0);

    tick += delay_tick; /* 计算下一个唤醒时刻 */
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
