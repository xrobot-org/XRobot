#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "component\ahrs.h"
#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\bmi088.h"
#include "device\can.h"

/* Exported constants --------------------------------------------------------*/
#define GIMBAL_OK (0)
#define GIMBAL_ERR (-1)
#define GIMBAL_ERR_MODE (-2)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

// TODO:改个一眼能看懂的名字
enum Gimbal_PID_e {
  GIMBAL_PID_YAW_IN = 0,
  GIMBAL_PID_YAW_OUT,
  GIMBAL_PID_PIT_IN,
  GIMBAL_PID_PIT_OUT,
  GIMBAL_PID_REL_YAW,
  GIMBAL_PID_REL_PIT,
  GIMBAL_PID_NUM,
};

enum Gimbal_Acuator_e {
  GIMBAL_ACTR_YAW = 0,
  GIMBAL_ACTR_PIT,
  GIMBAL_ACTR_NUM,
};

typedef struct {
  const PID_Params_t pid[GIMBAL_PID_NUM];
  float low_pass_cutoff;
  struct {
    struct {
      float high;
      float low;
    } pitch;
  } limit;
} Gimbal_Params_t;

typedef struct {
  Gimbal_Params_t *param;

  /* common */
  float dt_sec;
  CMD_Gimbal_Mode_t mode;

  struct {
    AHRS_Gyro_t gyro;

    struct {
      AHRS_Eulr_t *imu;
      AHRS_Eulr_t encoder;
    } eulr;
  } feedback;

  struct {
    AHRS_Eulr_t eulr;
  } set_point;

  PID_t pid[GIMBAL_PID_NUM];

  LowPassFilter2p_t filter[GIMBAL_ACTR_NUM];

  float out[GIMBAL_ACTR_NUM];

} Gimbal_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float dt_sec);
int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_t *can);
int8_t Gimbal_Control(Gimbal_t *g, CMD_Gimbal_Ctrl_t *g_ctrl);

#ifdef __cplusplus
}
#endif
