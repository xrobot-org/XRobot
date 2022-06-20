/*
 * 配置相关
 */

#include "mod_config.hpp"

#include <stdint.h>
#include <string.h>

#include "bsp_flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_SECTOR_11)

extern const config_robot_param_t param_robot;
extern const config_pilot_cfg_t cfg_pilot;
/**
 * @brief 从Flash读取配置信息
 *
 * @param cfg 配置信息
 */
void config_get(config_t *cfg) {
  bsp_flash_read_bytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));

  cfg->pilot_cfg = &cfg_pilot;
  cfg->robot_param = &param_robot;

  /* 防止第一次烧写后出现nan */
  if (isnanf(cfg->cali.bmi088.gyro_offset.x)) {
    memset(&(cfg->cali), 0, sizeof(cfg->cali));
  }

  if (isnanf(cfg->gimbal_mech_zero.pit)) {
    memset(&(cfg->gimbal_mech_zero), 0, sizeof(cfg->gimbal_mech_zero));
  }

  if (isnanf(cfg->gimbal_limit)) {
    cfg->gimbal_limit = 1.0f;
  }

  /* 确保配置有效，无内在冲突 */
  ASSERT(cfg->robot_param->chassis.reverse.yaw ==
         cfg->robot_param->gimbal.reverse.yaw);
}

/**
 * @brief 将配置信息写入Flash
 *
 * @param cfg 配置信息
 */
void config_set(config_t *cfg) {
  vTaskSuspendAll();
  bsp_flash_erase_sector(11);
  bsp_flash_write_bytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));
  xTaskResumeAll();
}
