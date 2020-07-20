/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

const osThreadAttr_t command_attr = {
  .name = "command",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 128 * 4,
};

const osThreadAttr_t ctrl_chassis_attr = {
  .name = "ctrl_chassis",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 256 * 4,
};

const osThreadAttr_t ctrl_gimbal_attr = {
  .name = "ctrl_gimbal",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 256 * 4,
};

const osThreadAttr_t ctrl_shoot_attr = {
  .name = "ctrl_shoot",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 256 * 4,
};

const osThreadAttr_t info_attr = {
  .name = "info",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 128 * 4,
};

const osThreadAttr_t monitor_attr = {
  .name = "monitor",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4,
};

const osThreadAttr_t pos_esti_attr = {
  .name = "pos_esti",
  .priority = (osPriority_t) osPriorityRealtime,
  .stack_size = 256 * 4,
};

const osThreadAttr_t referee_attr = {
  .name = "referee",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4,
};
