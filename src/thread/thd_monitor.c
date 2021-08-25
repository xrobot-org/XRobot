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
#include "bsp_adc.h"
#include "bsp_buzzer.h"
#include "bsp_led.h"
#include "bsp_usb.h"
#include "comp_capacity.h"
#include "thd.h"

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
void Thread_Monitor(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_MONITOR;

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    runtime.status.vbat = BSP_GetBatteryVolt(); /* ADC监测电压 */
    runtime.status.battery = Capacity_GetBatteryRemain(runtime.status.vbat);
    runtime.status.cpu_temp = BSP_GetTemperature();

    bool low_bat = runtime.status.battery < 0.5f;
    bool high_cpu_temp = runtime.status.cpu_temp > 50.0f;

    /* 电池电量少于20%时闪烁红色LED */
    if (low_bat || high_cpu_temp) {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_TAGGLE, 1);
    } else {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_OFF, 1);
    }

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
