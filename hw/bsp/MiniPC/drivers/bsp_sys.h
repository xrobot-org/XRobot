#pragma once

#include <sys/reboot.h>
#include <unistd.h>

#include "bsp_def.h"

/* 软件复位 */
__attribute__((always_inline, unused)) static inline void bsp_sys_reset(void) {
  sync();
  reboot(RB_AUTOBOOT);
}

/* 关机 */
__attribute__((always_inline, unused)) static inline void bsp_sys_shutdown(
    void) {
  sync();
  reboot(RB_POWER_OFF);
}

/* 睡眠模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_sleep(void) {
  sync();
  reboot(RB_SW_SUSPEND);
}

/* 停止模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_stop(void) {
  sync();
  reboot(RB_HALT_SYSTEM);
}

/* 中断状态 */
__attribute__((always_inline, unused)) static inline bool bsp_sys_in_isr(void) {
  return false;
}
