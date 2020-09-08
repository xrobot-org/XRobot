#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <cmsis_os2.h>

#include "component\user_math.h"
#include "device\device.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Motor */
typedef enum {
  CAN_MOTOR_M2006 = 0,
  CAN_MOTOR_M3508,
  CAN_MOTOR_GM6020,
} CAN_Motor_t;

typedef struct {
  float rotor_angle;
  float rotor_speed;
  float torque_current;
  float temp;
} CAN_MotorFeedback_t;

enum CAN_MotorChassis_e {
  CAN_MOTOR_CHASSIS_M1 = 0,
  CAN_MOTOR_CHASSIS_M2,
  CAN_MOTOR_CHASSIS_M3,
  CAN_MOTOR_CHASSIS_M4,
  CAN_MOTOR_CHASSIS_NUM,
};

enum CAN_MotorGimbal_e {
  CAN_MOTOR_GIMBAL_YAW = 0,
  CAN_MOTOR_GIMBAL_PIT,
  CAN_MOTOR_GIMBAL_NUM,
};

enum CAN_MotorShoot_e {
  CAN_MOTOR_SHOOT_FRIC1 = 0,
  CAN_MOTOR_SHOOT_FRIC2,
  CAN_MOTOR_SHOOT_TRIG,
  CAN_MOTORSHOOT_NUM,
};

typedef enum {
  CAN_MOTOR_GROUT_CHASSIS = 0,
  CAN_MOTOR_GROUT_GIMBAL1,
  CAN_MOTOR_GROUT_GIMBAL2,
  CAN_MOTOR_GROUT_SHOOT1,
  CAN_MOTOR_GROUT_SHOOT2,
  CAN_MOTOR_GROUT_NUM,
} CAN_MotorGroup_t;

typedef struct {
  uint8_t num;
  uint32_t id_start;
} CAN_MotorGroupInit_t;

typedef struct {
  CAN_MotorGroupInit_t chassis;
  CAN_MotorGroupInit_t gimbal1;
  CAN_MotorGroupInit_t gimbal2;
  CAN_MotorGroupInit_t shoot1;
  CAN_MotorGroupInit_t shoot2;
} CAN_MotorInit_t;

/* UWB */
typedef union {
  struct __packed {
    int16_t coor_x;
    int16_t coor_y;
    uint16_t yaw;
    int16_t distance[6];
    uint16_t err_mask : 14;
    uint16_t sig_level : 2;
    uint16_t reserved;
  } data;
} CAN_UWBFeedback_t;

/* Super capacitor */
typedef struct {
  uint16_t cap_volt;
  int16_t battery_volt;
  int16_t power_limit;
  uint8_t temp;
} CAN_CapFeedback_t;

/* CAN */
typedef struct {
  CAN_MotorInit_t *motor_init;
  osThreadId_t *motor_alert;
  uint8_t motor_alert_len;
  osThreadId_t uwb_alert;
  osThreadId_t cap_alert;

  CAN_MotorFeedback_t chassis_motor_fb[CAN_MOTOR_CHASSIS_NUM];
  CAN_MotorFeedback_t gimbal_motor_fb[CAN_MOTOR_GIMBAL_NUM];
  CAN_MotorFeedback_t shoot_motor_fb[CAN_MOTORSHOOT_NUM];

  CAN_UWBFeedback_t uwb_feedback;
  CAN_CapFeedback_t cap_feedback;
} CAN_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t CAN_Init(CAN_t *can_device, CAN_MotorInit_t *motor_init,
                osThreadId_t *motor_alert, uint8_t motor_alert_len,
                osThreadId_t uwb_alert, osThreadId_t cap_alert);

CAN_t *CAN_GetDevice(void);

int8_t CAN_Motor_Control(CAN_MotorGroup_t group, const float *out, uint8_t num);
int8_t CAN_Motor_ControlChassis(float m1, float m2, float m3, float m4);
int8_t CAN_Motor_ControlGimbal(float yaw, float pitch);
int8_t CAN_Motor_ControlShoot(float fric1, float fric2, float trig);

int8_t CAN_Motor_QuickIdSetMode(void);
int8_t CAN_CapControl(float power_limit);

#ifdef __cplusplus
}
#endif
