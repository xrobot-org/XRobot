#pragma once

#include <webots/robot.h>
#include <webots/supervisor.h>

#include "bsp_def.h"

/* 软件复位 */
__attribute__((always_inline, unused)) static inline void bsp_sys_reset(void) {
  wb_supervisor_simulation_reset();
}

/* 关机 */
__attribute__((always_inline, unused)) static inline void bsp_sys_shutdown(
    void) {
  wb_supervisor_simulation_quit(EXIT_SUCCESS);  // ask Webots to terminate
  wb_robot_cleanup();
}

/* 睡眠模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_sleep(void) {
  wb_supervisor_simulation_set_mode(WB_SUPERVISOR_SIMULATION_MODE_PAUSE);
}

/* 停止模式 */
__attribute__((always_inline, unused)) static inline void bsp_sys_stop(void) {
  wb_supervisor_simulation_set_mode(WB_SUPERVISOR_SIMULATION_MODE_PAUSE);
}

/* 中断状态 */
__attribute__((always_inline, unused)) static inline bool bsp_sys_in_isr(void) {
  return false;
}
