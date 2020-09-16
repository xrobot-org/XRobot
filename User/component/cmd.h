/*
        控制命令
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "ahrs.h"

typedef enum {
  /* No force applied. For all robot when power on. */
  CHASSIS_MODE_RELAX,
  /* Set to zero speed. Force applied. For all robot when break. */
  CHASSIS_MODE_BREAK,
  /* Follow gimbal by follow encoder. For infantry, hero and engineer. */
  CHASSIS_MODE_FOLLOW_GIMBAL,
  /* Constantly rotating. For infantry and hero. */
  CHASSIS_MODE_ROTOR,
  /* Run independently. For sentry and drone. */
  CHASSIS_MODE_INDENPENDENT,
  /* Direct apply force without pid control. For TEST only. */
  CHASSIS_MODE_OPEN,
} CMD_Chassis_Mode_t;

typedef enum {
  /* No force applied. */
  GIMBAL_MODE_RELAX,
  /* Follow IMU data. */
  GIMBAL_MODE_ABSOLUTE,
  /* Follow encoder data. */
  GIMBAL_MODE_RELATIVE,
  /* Set to a fix angle. Force applied. */
  GIMBAL_MODE_FIX,
} CMD_Gimbal_Mode_t;

typedef enum {
  /* No force applied. */
  SHOOT_MODE_RELAX,
  /* Set to zero speed. Force applied. */
  SHOOT_MODE_SAFE,
  /* Safty off. */
  SHOOT_MODE_STDBY,
  /* Shooting. */
  SHOOT_MODE_FIRE,
} CMD_Shoot_Mode_t;

typedef struct {
  CMD_Chassis_Mode_t mode;
  MoveVector_t ctrl_v;
} CMD_Chassis_Ctrl_t;

typedef struct {
  CMD_Gimbal_Mode_t mode;
  AHRS_Eulr_t delta_eulr;
} CMD_Gimbal_Ctrl_t;

typedef struct {
  CMD_Shoot_Mode_t mode;
  float bullet_speed;
  float shoot_freq_hz;
} CMD_Shoot_Ctrl_t;

typedef enum {
  CMD_SW_ERR = 0,
  CMD_SW_UP = 1,
  CMD_SW_MID = 3,
  CMD_SW_DOWN = 2,
} CMD_SwitchPos_t;

typedef enum {
  CMD_KEY_W = 0,
  CMD_KEY_S,
  CMD_KEY_A,
  CMD_KEY_D,
  CMD_KEY_Q,
  CMD_KEY_E,
  CMD_KEY_SHIFT,
  CMD_KEY_CTRL,
} CMD_KeyValue_t;

typedef struct {
  float sens_mouse;
  float sens_rc;
} CMD_Params_t;

typedef struct {
  bool pc_ctrl;

  const CMD_Params_t *param;

  CMD_Chassis_Ctrl_t chassis;
  CMD_Gimbal_Ctrl_t gimbal;
  CMD_Shoot_Ctrl_t shoot;
} CMD_t;

typedef struct {
  float ch_l_x;
  float ch_l_y;
  float ch_r_x;
  float ch_r_y;

  float ch_res;

  CMD_SwitchPos_t sw_l;
  CMD_SwitchPos_t sw_r;

  struct {
    int16_t x;
    int16_t y;
    int16_t z;
    bool l_click;
    bool r_click;
  } mouse;

  uint16_t key;
  uint16_t res;
} CMD_RC_t;

int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param);
int8_t CMD_Parse(const CMD_RC_t *rc, CMD_t *cmd);

#ifdef __cplusplus
}
#endif
