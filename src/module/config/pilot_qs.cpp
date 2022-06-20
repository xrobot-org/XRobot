#include "mod_config.hpp"

#if PL_QS

/* clang-format off */
extern const config_pilot_cfg_t cfg_pilot = {
  .name = "qs",
  .param = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map = {
        {CMD_ACTIVE_PRESSED, CMD_KEY_W},        /* 向前 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_S},        /* 向后 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_A},        /* 向左 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_D},        /* 向右 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_SHIFT},    /* 加速 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_CTRL},     /* 减速 */
        {CMD_ACTIVE_PRESSED, CMD_KEY_L_CLICK},  /* 开火 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_R_CLICK}, /* 切换开火模式 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_E},       /* 自瞄模式 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_F},       /* 弹舱盖开关 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_R},       /* 小陀螺模式 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_G},       /* 反转拨弹 */
        {CMD_ACTIVE_PRESSING, CMD_KEY_C}, },    /* 跟随云台呈35度 */
    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },
  },
  .screen = {
    .width = 1920,
    .height = 1080,
  },
};
/* clang-format on */

#endif
