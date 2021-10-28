/**
 * @file monitor.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 监控线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 监控系统运行情况，记录错误。
 *
 */

#include "bsp_usb.h"
#include "comp_capacity.h"
#include "dev_adc.h"
#include "dev_buzzer.h"
#include "dev_led.h"
#include "thd.h"

#define THD_PERIOD_MS (100)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void Thd_Monitor(void* arg) {
  Runtime_t* runtime = arg;

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    runtime->status.vbat = Volt_GetBattery(); /* ADC监测电压 */
    runtime->status.battery = Capacity_GetBatteryRemain(runtime->status.vbat);
    runtime->status.cpu_temp = Temperature_GetCPU();

    bool low_bat = runtime->status.battery < 0.5f;
    bool high_cpu_temp = runtime->status.cpu_temp > 50.0f;

    /* 电池电量少于20%时闪烁红色LED */
    if (low_bat || high_cpu_temp) {
      LED_Set(LED_RED, LED_TAGGLE, 1);
    } else {
      LED_Set(LED_RED, LED_OFF, 1);
    }

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
