#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "comp_ahrs.h"
#include "comp_utils.h"
#include "dev.h"
#include "queue.h"

/* Motor id */
/* id     feedback id     control id */
/* 1-4    0x205 to 0x208  0x1ff */
/* 5-6    0x209 to 0x20B  0x2ff */
#define GM6020_FB_ID_BASE (0x205)
#define GM6020_CTRL_ID_BASE (0x1ff)
#define GM6020_CTRL_ID_EXTAND (0x2ff)

/* id     feedback id		  control id */
/* 1-4		0x201 to 0x204  0x200 */
/* 5-6		0x205 to 0x208  0x1ff */
#define M3508_M2006_FB_ID_BASE (0x201)
#define M3508_M2006_CTRL_ID_BASE (0x200)
#define M3508_M2006_CTRL_ID_EXTAND (0x1ff)
#define M3508_M2006_ID_SETTING_ID (0x700)

/* 电机型号 */
typedef enum {
  MOTOR_NONE = 0,
  MOTOR_M2006,
  MOTOR_M3508,
  MOTOR_GM6020,
} Motor_Model_t;

typedef struct {
  uint32_t id_feedback;
  uint32_t id_control;
  Motor_Model_t model[4];
  uint8_t num;
} Motor_Group_t;

typedef union {
  float as_array[4];

  struct {
    float m1;
    float m2;
    float m3;
    float m4;
  } as_chassis;

  AHRS_Eulr_t as_gimbal;

  struct {
    float fric_left;
    float fric_right;
    float trig;
  } as_launcher;
} Motor_Control_t;

/* 电机反馈信息 */
typedef struct {
  float rotor_abs_angle;  /* 转子绝对角度 单位：rad */
  float rotational_speed; /* 转速 单位：rpm */
  float torque_current;   /* 转矩电流 单位：A*/
  float temp;             /* 电机温度 单位：℃*/
} Motor_Feedback_t;

typedef union {
  Motor_Feedback_t as_array[4];

  struct {
    Motor_Feedback_t m1;
    Motor_Feedback_t m2;
    Motor_Feedback_t m3;
    Motor_Feedback_t m4;
  } as_chassis;

  struct {
    Motor_Feedback_t yaw;
    Motor_Feedback_t pit;
    Motor_Feedback_t rol;
  } as_gimbal;

  struct {
    Motor_Feedback_t fric_left;
    Motor_Feedback_t fric_right;
    Motor_Feedback_t trig;
  } as_launcher;
} Motor_FeedbackGroup_t;

typedef enum {
  MOTOR_GROUT_ID_CHASSIS = 0,
  MOTOR_GROUT_ID_GIMBAL1,
  MOTOR_GROUT_ID_GIMBAL2,
  MOTOR_GROUT_ID_LAUNCHER1,
  MOTOR_GROUT_ID_LAUNCHER2,
  MOTOR_GROUT_ID_NUM,
} Motor_GroupID_t;

typedef struct {
  Motor_FeedbackGroup_t feedback[MOTOR_GROUT_ID_NUM];
  QueueHandle_t msgq_tx;
  QueueHandle_t msgq_rx;

  const Motor_Group_t *group_cfg;
} Motor_t;

Err_t Motor_Init(Motor_t *motor, const Motor_Group_t *group_cfg);
Err_t Motor_Update(Motor_t *motor, uint32_t timeout);
Err_t Motor_Control(Motor_t *motor, Motor_GroupID_t group,
                    Motor_Control_t *output);
Err_t Motor_HandleOffline(Motor_t *motor);
