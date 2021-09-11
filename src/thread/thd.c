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

extern void Thd_AI(void *argument);
extern void Thd_AttiEsti(void *argument);
extern void Thd_CAN(void *argument);
extern void Thd_CLI(void *argument);
extern void Thd_CMD(void *argument);
extern void Thd_CtrlCap(void *argument);
extern void Thd_CtrlChassis(void *argument);
extern void Thd_CtrlGimbal(void *argument);
extern void Thd_CtrlLauncher(void *argument);
extern void Thd_IMU(void *argument);
extern void Thd_Info(void *argument);
extern void Thd_Monitor(void *argument);
extern void Thd_MsgDistrib(void *argument);
extern void Thd_RC(void *argument);
extern void Thd_Referee(void *argument);
extern void Thd_USB(void *argument);

/* 机器人运行时的数据 */
Runtime_t runtime;

typedef struct {
  TaskFunction_t fn;
  const char *name;
  configSTACK_DEPTH_TYPE stack_depth;
  UBaseType_t priority;
  Thd_Name_t handle_name;
} Thd_t;

static const Thd_t thd_list[] = {
    {Thd_AI, "AI", 128, 5, THD_AI},
    {Thd_AttiEsti, "AttiEsti", 256, 5, THD_ATTI_ESTI},
    {Thd_CAN, "CAN", 128, 5, THD_CAN},
    {Thd_CLI, "CLI", 256, 3, THD_CLI},
    {Thd_CMD, "CMD", 128, 4, THD_CMD},
    {Thd_CtrlCap, "CtrlCap", 128, 3, THD_CTRL_CAP},
    {Thd_CtrlChassis, "CtrlChassis", 256, 3, THD_CTRL_CHASSIS},
    {Thd_CtrlGimbal, "CtrlGimbal", 256, 3, THD_CTRL_GIMBAL},
    {Thd_CtrlLauncher, "CtrlLauncher", 256, 3, THD_CTRL_LAUNCHER},
    {Thd_IMU, "IMU", 256, 3, THD_IMU},
    {Thd_Info, "Info", 128, 2, THD_INFO},
    {Thd_Monitor, "Monitor", 128, 2, THD_MONITOR},
    {Thd_MsgDistrib, "MsgDistrib", 128, 2, THD_MSG_DISTRIB},
    {Thd_RC, "RC", 128, 5, THD_RC},
    {Thd_Referee, "Referee", 512, 5, THD_REFEREE},
    {Thd_USB, "USB", 128, 5, THD_USB},
};

void Thd_Init(void) {
  Config_Get(&runtime.cfg); /* 获取机器人配置 */

  vTaskSuspendAll();
  /* 创建线程 */
  for (size_t j = 0; j < ARRAY_LEN(thd_list); j++) {
    const Thd_t *thd = thd_list + j;
    xTaskCreate(thd->fn, thd->name, thd->stack_depth, &runtime, thd->priority,
                runtime.thd + thd->handle_name);
  }
  xTaskResumeAll();
}
