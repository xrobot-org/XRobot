#pragma once

#include "bsp_def.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "portable.h"

/* 软件复位 */
__attribute__((always_inline, unused)) static inline void bsp_sys_reset(void) {
  esp_restart();
}

/* 关机 */
__attribute__((always_inline, unused)) static inline void bsp_sys_shutdown(
    void) {
  esp_deep_sleep_start();
}

/* 睡眠模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_sleep(void) {
  esp_light_sleep_start();
}

/* 停止模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_stop(void) {
  esp_deep_sleep_start();
}

/* 中断状态 */
__attribute__((always_inline, unused)) static inline bool bsp_sys_in_isr(void) {
  return xPortInIsrContext();
}
