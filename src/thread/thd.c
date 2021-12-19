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

/* 机器人运行时的数据 */
runtime_t runtime;

extern const thd_t _sthread, _ethread;

static TaskHandle_t *thd_list;

void thd_init(void) {
  config_get(&runtime.cfg); /* 获取机器人配置 */

  const thd_t *start_addr = &_sthread;
  const thd_t *end_addr = &_ethread;

  const size_t num_thread = end_addr - start_addr;
  thd_list = pvPortMalloc(num_thread * sizeof(TaskHandle_t));

  /* 创建线程 */
  vTaskSuspendAll();
  for (size_t j = 0; j < num_thread; j++) {
    const thd_t *thd = start_addr + j;
    if (thd) {
      xTaskCreate(thd->fn, thd->name, thd->stack_depth, &runtime, thd->priority,
                  thd_list + j);
    } else {
      break;
    }
  }
  xTaskResumeAll();
}
