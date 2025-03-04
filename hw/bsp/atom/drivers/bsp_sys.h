#pragma once

#include "bsp_def.h"
#include "bsp_pwm.h"
#include "main.h"

__attribute__((section(".reboot_in_bootloader_section"), used)) static void
bsp_reboot_in_bootloader(void);

/* 软件复位 */
__attribute__((always_inline, unused)) static inline void bsp_sys_reset(void) {
  __set_FAULTMASK(1);
  NVIC_SystemReset();
  bsp_reboot_in_bootloader();
}

/* 关机 */
__attribute__((always_inline, unused)) static inline void bsp_sys_shutdown(
    void) {
  HAL_PWR_EnterSTANDBYMode();
}

/* 睡眠模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_sleep(void) {
  HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

/* 停止模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_stop(void) {
  HAL_PWR_EnterSTANDBYMode();
}

/* 中断状态 */
__attribute__((always_inline, unused)) static inline bool bsp_sys_in_isr(void) {
  uint32_t result;
  __asm__ volatile("MRS %0, ipsr" : "=r"(result));
  return (result);
}

__attribute__((section(".reboot_in_bootloader_section"), used)) static void
bsp_reboot_in_bootloader(void) {
  NVIC_SystemReset();
}
