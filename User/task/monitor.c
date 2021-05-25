/**
 * @file monitor.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 监控任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 监控系统运行情况，记录错误。
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "bsp/adc.h"
#include "bsp/buzzer.h"
#include "bsp/led.h"
#include "bsp/usb.h"
#include "component/capacity.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 监控
 *
 * @param argument 未使用
 */
void Task_Monitor(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_MONITOR;

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    runtime.stack_water_mark.monitor = osThreadGetStackSpace(osThreadGetId());
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */
    runtime.status.vbat = BSP_GetBatteryVolt(); /* ADC监测电压 */
    runtime.status.battery = Capacity_GetBatteryRemain(runtime.status.vbat);
    runtime.status.cpu_temp = BSP_GetTemperature();

    bool low_bat = runtime.status.battery < 0.2f;
    bool high_cpu_temp = runtime.status.cpu_temp > 50.0f;

    /* 电池电量少于20%时闪烁红色LED */
    if (low_bat || high_cpu_temp) {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_TAGGLE, 1);
    } else {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_OFF, 1);
    }
    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
