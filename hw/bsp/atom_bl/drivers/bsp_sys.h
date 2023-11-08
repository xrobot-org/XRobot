#pragma once

#include "bsp_def.h"
#include "main.h"
#include "stm32g4xx_hal_uart.h"

/* 软件复位 */
__attribute__((always_inline, unused)) static inline void bsp_sys_reset(void) {
  __set_FAULTMASK(1);
  NVIC_SystemReset();
}

/* 关机 */
__attribute__((always_inline, unused)) static inline void bsp_sys_shutdown(
    void) {
  HAL_PWR_EnterSTANDBYMode();
}

/* 进入Bootloader */
__attribute__((always_inline, unused)) static inline void bsp_sys_bootloader(
    void) {
  __enable_irq();
  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = SysTick->LOAD = SysTick->VAL = 0;
  __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

  const uint32_t p = (*((uint32_t *)0x1FFF0000));
  __set_MSP(p);

  void (*SysMemBootJump)(void);
  SysMemBootJump = (void (*)(void))(*((uint32_t *)0x1FFF0004));
  SysMemBootJump();

  NVIC_SystemReset();
}

/* 进入APP */
__attribute__((always_inline, unused)) static inline void bsp_sys_jump_app(
    void) {
  HAL_RCC_DeInit();
  HAL_DeInit();
  __set_MSP(0x08005000);
  SCB->VTOR = 0x08005000;
  static void (*APP_FUN)();
  APP_FUN = *(void (**)())(0x08005004);
  APP_FUN();
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
