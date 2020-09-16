#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <cmsis_os2.h>

#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\can.h"

/* Exported constants --------------------------------------------------------*/
#define SHOOT_OK (0)
#define SHOOT_ERR (-1)
#define SHOOT_ERR_MODE (-2)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
enum Shoot_Acuator_e {
  SHOOT_ACTR_FRIC1 = 0,
  SHOOT_ACTR_FRIC2,
  SHOOT_ACTR_TRIG,
  SHOOT_ACTR_NUM,
};

typedef struct {
  const PID_Params_t fric_pid_param[2];
  PID_Params_t trig_pid_param;

  struct {
    float fric;
    float trig;
  } low_pass_cutoff;
} Shoot_Params_t;

typedef struct {
  const Shoot_Params_t *param;

  /* common */
  float dt_sec;
  CMD_Shoot_Mode_t mode;
  osTimerId_t trig_timer_id;

  struct {
    float fric_rpm[2];
    float trig_angle;
  } feedback;

  struct {
    float fric_rpm[2];
    float trig_angle;
  } set_point;

  struct {
    PID_t fric[2];
    PID_t trig;
  } pid;

  struct {
    LowPassFilter2p_t fric[2];
    LowPassFilter2p_t trig;
  } filter;

  int8_t heat_limiter;

  float out[SHOOT_ACTR_NUM];

} Shoot_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float dt_sec);
int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_t *can);
int8_t Shoot_Control(Shoot_t *s, CMD_Shoot_Ctrl_t *s_ctrl);

#ifdef __cplusplus
}
#endif
