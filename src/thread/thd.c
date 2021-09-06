/**
 * @file user_task.c
 * @author Qu Shen (503578404@qq.com)
 * @brief
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 保存线程属性：堆栈大小、优先级等
 * 生成线程时使用。
 *
 * @note 所有直接处理物理设备的线程（CAN、DR16等）优先级应该最高
 * 设备之间有心计可以不一样，但是需要极其小心
 * 运行模块（module）线程的优先级应该低于物理设备线程优先级
 * 其他辅助运行的非核心功能应该更低
 *
 */

#include "thd.h"

#include "bsp_flash.h"
#include "comp_cmd.h"
#include "dev_ai.h"
#include "dev_bmi088.h"
#include "dev_can.h"
#include "dev_ist8310.h"
#include "dev_referee.h"
#include "mod_cap.h"
#include "mod_chassis.h"
#include "mod_gimbal.h"
#include "mod_launcher.h"

void Thread_AI(void *argument);
void Thread_AttiEsti(void *argument);
void Thread_CAN(void *argument);
void Thread_CLI(void *argument);
void Thread_CMD(void *argument);
void Thread_CtrlCap(void *argument);
void Thread_CtrlChassis(void *argument);
void Thread_CtrlGimbal(void *argument);
void Thread_CtrlLauncher(void *argument);
void Thread_Info(void *argument);
void Thread_Monitor(void *argument);
void Thread_RC(void *argument);
void Thread_Referee(void *argument);

/* 机器人运行时的数据 */
Runtime_t runtime;

/**
 * @brief 初始化
 *
 * @param argument 未使用
 */
void Thread_Init(void) {
  Config_Get(&runtime.cfg); /* 获取机器人配置 */

  vTaskSuspendAll();
  /* 创建线程, 优先级随数字增大而增大 */
  xTaskCreate(Thread_AttiEsti, "AttiEsti", 256, &runtime, 5,
              &runtime.thread.atti_esti);
  xTaskCreate(Thread_CLI, "CLI", 256, &runtime, 3, &runtime.thread.cli);
  xTaskCreate(Thread_CMD, "CMD", 128, &runtime, 4, &runtime.thread.cmd);
  xTaskCreate(Thread_CtrlCap, "CtrlCap", 128, &runtime, 3,
              &runtime.thread.ctrl_cap);
  xTaskCreate(Thread_CtrlChassis, "CtrlChassis", 256, &runtime, 3,
              &runtime.thread.ctrl_chassis);
  xTaskCreate(Thread_CtrlGimbal, "CtrlGimbal", 256, &runtime, 3,
              &runtime.thread.ctrl_gimbal);
  xTaskCreate(Thread_CtrlLauncher, "CtrlLauncher", 256, &runtime, 3,
              &runtime.thread.ctrl_launcher);
  xTaskCreate(Thread_Info, "Info", 128, &runtime, 2, &runtime.thread.info);
  xTaskCreate(Thread_Monitor, "Monitor", 128, &runtime, 2,
              &runtime.thread.monitor);
  xTaskCreate(Thread_CAN, "CAN", 128, &runtime, 5, &runtime.thread.can);
  xTaskCreate(Thread_Referee, "Referee", 512, &runtime, 5,
              &runtime.thread.referee);
  xTaskCreate(Thread_AI, "AI", 128, &runtime, 5, &runtime.thread.ai);
  xTaskCreate(Thread_RC, "RC", 128, &runtime, 5, &runtime.thread.rc);

  xTaskResumeAll();
  vTaskDelete(NULL); /* 结束自身 */
}
