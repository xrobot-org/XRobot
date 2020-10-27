/*
  监控任务。

  监控系统运行情况，记录错误。
*/

/* Includes ----------------------------------------------------------------- */
#include "bsp\adc.h"
#include "bsp\buzzer.h"
#include "bsp\led.h"
#include "bsp\usb.h"
#include "component/capacity.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 监控
 *
 * \param argument 未使用
 */
void Task_Monitor(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_MONITOR;

  osDelay(TASK_INIT_DELAY_REFEREE); /* 延时一段时间再开启任务 */

  uint32_t tick = osKernelGetTickCount(); /* 控制任务运行频率的计时 */
  while (1) {
#ifdef DEBUG
    /* 记录任务所使用的的栈空间 */
    task_runtime.stack_water_mark.monitor = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick; /* 计算下一个唤醒时刻 */
    float battery_volt = BSP_GetBatteryVolt();
    task_runtime.status.battery = Capacity_GetBatteryRemain(battery_volt);

    /* 电池电量少于20%时闪烁红色LED */
    if (task_runtime.status.battery < 0.2f) {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_TAGGLE, 1);
    } else {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_OFF, 1);
    }

    task_runtime.status.cpu_temp = BSP_GetTemperature();

    /* CPU温度高于35℃时时闪烁蓝色LED */
    if (task_runtime.status.cpu_temp > 35.0f) {
      BSP_LED_Set(BSP_LED_BLU, BSP_LED_ON, 1);
    } else {
      BSP_LED_Set(BSP_LED_BLU, BSP_LED_OFF, 1);
    }

    osDelayUntil(tick); /* 运行结束，等待下一次唤醒 */
  }
}
