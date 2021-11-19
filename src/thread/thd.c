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
#include "mod_config.h"
#include "task.h"

extern void Thd_AI(void *arg);
extern void Thd_AttiEsti(void *arg);
extern void Thd_Cap(void *arg);
extern void Thd_CLI(void *arg);
extern void Thd_CMD(void *arg);
extern void Thd_CtrlChassis(void *arg);
extern void Thd_CtrlGimbal(void *arg);
extern void Thd_CtrlLauncher(void *arg);
extern void Thd_IMU(void *arg);
extern void Thd_Info(void *arg);
extern void Thd_Monitor(void *arg);
extern void Thd_Motor(void *arg);
extern void Thd_MsgDist(void *arg);
extern void Thd_RC(void *arg);
extern void Thd_Referee(void *arg);
extern void Thd_TOF(void *arg);
extern void Thd_USB(void *arg);

/* 机器人运行时的数据 */
Runtime_t runtime;

extern const Thd_t *__thread_start;
extern const Thd_t *__thread_end;

static TaskHandle_t *thd_list;

void Thd_Init(void) {
  Config_Get(&runtime.cfg); /* 获取机器人配置 */

  vTaskSuspendAll();
  const size_t num_thread = (__thread_end - __thread_start) / sizeof(Thd_t);
  thd_list = pvPortMalloc(num_thread * sizeof(TaskHandle_t));

  /* 创建线程 */
  for (size_t j = 0; j < num_thread; j++) {
    const Thd_t *thd = __thread_start + j;
    if (thd) {
      xTaskCreate(thd->fn, thd->name, thd->stack_depth, &runtime, thd->priority,
                  thd_list + j);
    }
  }
  xTaskResumeAll();
}
