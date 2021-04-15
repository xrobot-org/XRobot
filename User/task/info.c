/**
 * @file info.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 指示任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 控制指示装置，例如LED、OLED显示器等。
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "bsp/led.h"
#include "bsp/usb.h"
#include "component/capacity.h"
#include "component/user_math.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 信息
 *
 * \param argument 未使用
 */
void Task_Info(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_INFO;

  osDelay(TASK_INIT_DELAY_INFO); /* 延时一段时间再开启任务 */

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.info = osThreadGetStackSpace(osThreadGetId());
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */

    BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1); /* 闪烁LED */

    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
