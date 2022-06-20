/**
 * @file info.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 指示线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 控制指示装置，例如LED、OLED显示器等。
 *
 */

#include "bsp_adc.h"
#include "comp_capacity.hpp"
#include "comp_utils.hpp"
#include "dev_buzzer.hpp"
#include "dev_led.hpp"
#include "dev_rgb.hpp"
#include "thd.hpp"

#define THD_PERIOD_MS (100)

/* 计算线程运行到指定频率需要等待的tick数 */
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_info(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;

  uint32_t previous_wake_time = xTaskGetTickCount();

  uint8_t led_fsm = 0;

  while (1) {
    runtime->status.vbat = bsp_adc_get_batt_volt(); /* ADC监测电压 */
    runtime->status.battery = capacity_get_battery_remain(runtime->status.vbat);
    runtime->status.cpu_temp = bsp_adc_get_cpu_temp();

    switch (led_fsm) {
      case 0:
        led_set(LED_GRN, LED_ON, 1);
        led_set(LED_RED, LED_OFF, 1);
        led_set(LED_BLU, LED_OFF, 1);
        led_fsm++;
        break;
      case 1:
        led_set(LED_GRN, LED_OFF, 1);
        led_set(LED_RED, LED_ON, 1);
        led_set(LED_BLU, LED_OFF, 1);
        led_fsm++;
        break;
      case 2:
        led_set(LED_GRN, LED_OFF, 1);
        led_set(LED_RED, LED_OFF, 1);
        led_set(LED_BLU, LED_ON, 1);
        led_fsm = 0;
        break;
    }

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_info, 128, 1);
