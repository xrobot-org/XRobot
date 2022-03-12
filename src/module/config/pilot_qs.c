#include "mod_config.h"

#if PL_QS

/* clang-format off */
const config_pilot_cfg_t cfg_pilot = {
  .name = "qs",
  .param = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map[CMD_BEHAVIOR_FORE] = {CMD_ACTIVE_PRESSED, CMD_KEY_W},
    .key_map[CMD_BEHAVIOR_BACK] = {CMD_ACTIVE_PRESSED, CMD_KEY_S},
    .key_map[CMD_BEHAVIOR_LEFT] = {CMD_ACTIVE_PRESSED, CMD_KEY_A},
    .key_map[CMD_BEHAVIOR_RIGHT] = {CMD_ACTIVE_PRESSED, CMD_KEY_D},
    .key_map[CMD_BEHAVIOR_ACCELERATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_SHIFT},
    .key_map[CMD_BEHAVIOR_DECELEBRATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_CTRL},
    .key_map[CMD_BEHAVIOR_FIRE] = {CMD_ACTIVE_PRESSED, CMD_KEY_L_CLICK},
    .key_map[CMD_BEHAVIOR_FIRE_MODE] = {CMD_ACTIVE_PRESSING, CMD_KEY_R_CLICK},
    .key_map[CMD_BEHAVIOR_FOLLOWGIMBAL35] = {CMD_ACTIVE_PRESSING, CMD_KEY_E},
    .key_map[CMD_BEHAVIOR_OPENCOVER] =  {CMD_ACTIVE_PRESSING, CMD_KEY_F},
    .key_map[CMD_BEHAVIOR_REVTRIG] = {CMD_ACTIVE_PRESSING, CMD_KEY_R},
    .key_map[CMD_BEHAVIOR_ROTOR] = {CMD_ACTIVE_PRESSING, CMD_KEY_G},
    .key_map[CMD_BEHAVIOR_AUTOAIM] = {CMD_ACTIVE_PRESSING, CMD_KEY_C},
    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },
  },
  .screen = {
    .height = 1080,
    .width = 1920,
  },
};
/* clang-format on */

#endif
