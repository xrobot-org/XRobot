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
 * 设备之间优先级可以不一样，但是需要极其小心
 * 运行模块（module）线程的优先级应该低于物理设备线程优先级
 * 其他辅助运行的非核心功能应该更低
 *
 */

#include "thd.h"

#include "FreeRTOS.h"
#include "comp_utils.h"
#include "task.h"

extern void Thread_AI(void *argument);
extern void Thread_AttiEsti(void *argument);
extern void Thread_CAN(void *argument);
extern void Thread_CLI(void *argument);
extern void Thread_CMD(void *argument);
extern void Thread_CtrlCap(void *argument);
extern void Thread_CtrlChassis(void *argument);
extern void Thread_CtrlGimbal(void *argument);
extern void Thread_CtrlLauncher(void *argument);
extern void Thread_IMU(void *argument);
extern void Thread_Info(void *argument);
extern void Thread_Monitor(void *argument);
extern void Thread_MsgDistrib(void *argument);
extern void Thread_RC(void *argument);
extern void Thread_Referee(void *argument);
extern void Thread_USB(void *argument);

/* 机器人运行时的数据 */
Runtime_t runtime;

typedef struct {
  TaskFunction_t thrad_fn;
  const char *name;
  configSTACK_DEPTH_TYPE stack_depth;
  UBaseType_t priority;
  Thread_Name_t handle_name;
} Thread_t;

static const Thread_t thread_list[] = {
    {Thread_AI, "AI", 128, 5, THREAD_AI},
    {Thread_AttiEsti, "AttiEsti", 256, 5, THREAD_ATTI_ESTI},
    {Thread_CAN, "CAN", 128, 5, THREAD_CAN},
    {Thread_CLI, "CLI", 256, 3, THREAD_CLI},
    {Thread_CMD, "CMD", 128, 4, THREAD_CMD},
    {Thread_CtrlCap, "CtrlCap", 128, 3, THREAD_CTRL_CAP},
    {Thread_CtrlChassis, "CtrlChassis", 256, 3, THREAD_CTRL_CHASSIS},
    {Thread_CtrlGimbal, "CtrlGimbal", 256, 3, THREAD_CTRL_GIMBAL},
    {Thread_CtrlLauncher, "CtrlLauncher", 256, 3, THREAD_CTRL_LAUNCHER},
    {Thread_IMU, "IMU", 256, 3, THREAD_IMU},
    {Thread_Info, "Info", 128, 2, THREAD_INFO},
    {Thread_Monitor, "Monitor", 128, 2, THREAD_MONITOR},
    {Thread_MsgDistrib, "MsgDistrib", 128, 2, THREAD_MSG_DISTRIB},
    {Thread_RC, "RC", 128, 5, THREAD_RC},
    {Thread_Referee, "Referee", 512, 5, THREAD_REFEREE},
    {Thread_USB, "USB", 128, 5, THREAD_USB},
};

/**
 * @brief 初始化
 *
 * @param argument 未使用
 */
void Thread_Init(void) {
  Config_Get(&runtime.cfg); /* 获取机器人配置 */

  vTaskSuspendAll();
  /* 创建线程 */
  for (size_t j = 0; j < ARRAY_LEN(thread_list); j++) {
    Thread_t *thread = thread_list + j;
    xTaskCreate(thread->thrad_fn, thread->name, thread->stack_depth, &runtime,
                thread->priority, &runtime.thread[thread->handle_name]);
  }
  xTaskResumeAll();
}
