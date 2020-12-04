#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdbool.h>

#include "component\ahrs.h"
#include "component\user_math.h"
#include "device\device.h"

/* Exported constants ------------------------------------------------------- */
#define CAN_MOTOR_CHASSIS_1_RECV (1 << 0)
#define CAN_MOTOR_CHASSIS_2_RECV (1 << 1)
#define CAN_MOTOR_CHASSIS_3_RECV (1 << 2)
#define CAN_MOTOR_CHASSIS_4_RECV (1 << 3)
#define CAN_MOTOR_GIMBAL_YAW_RECV (1 << 4)
#define CAN_MOTOR_GIMBAL_PIT_RECV (1 << 5)
#define CAN_MOTOR_SHOOT_FRIC1_RECV (1 << 6)
#define CAN_MOTOR_SHOOT_FRIC2_RECV (1 << 7)
#define CAN_MOTOR_SHOOT_TRIG_RECV (1 << 8)

#define MOTOR_REC_CHASSIS_FINISHED                       \
  (CAN_MOTOR_CHASSIS_1_RECV | CAN_MOTOR_CHASSIS_2_RECV | \
   CAN_MOTOR_CHASSIS_3_RECV | CAN_MOTOR_CHASSIS_4_RECV)
#define MOTOR_REC_GIMBAL_FINISHED \
  (CAN_MOTOR_GIMBAL_YAW_RECV | CAN_MOTOR_GIMBAL_PIT_RECV)
#define MOTOR_REC_SHOOT_FINISHED                             \
  (CAN_MOTOR_SHOOT_FRIC1_RECV | CAN_MOTOR_SHOOT_FRIC2_RECV | \
   CAN_MOTOR_SHOOT_TRIG_RECV)

#define CAN_MOTOR_TX_BUF_SIZE (8)
#define CAN_MOTOR_RX_BUF_SIZE (8)

#define CAN_CAP_TX_BUF_SIZE (8)
#define CAN_CAP_RX_BUF_SIZE (8)

/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 电机型号 */
typedef enum {
  CAN_MOTOR_M2006 = 0,
  CAN_MOTOR_M3508,
  CAN_MOTOR_GM6020,
} CAN_MotorModel_t;

typedef enum {
  CAN_M3508_M1_ID = 0x201, /* 1 */
  CAN_M3508_M2_ID = 0x202, /* 2 */
  CAN_M3508_M3_ID = 0x203, /* 3 */
  CAN_M3508_M4_ID = 0x204, /* 4 */

  CAN_M3508_FRIC1_ID = 0x205, /* 5 */
  CAN_M3508_FRIC2_ID = 0x206, /* 6 */
  CAN_M2006_TRIG_ID = 0x207,  /* 7 */

  CAN_GM6020_YAW_ID = 0x209, /* 5 */
  CAN_GM6020_PIT_ID = 0x20A, /* 6 */
} CAN_MotorId_t;

/* 电机反馈信息 */
typedef struct {
  float rotor_angle;
  float rotor_speed;
  float torque_current;
  float temp;
} CAN_MotorFeedback_t;

/* 电机 */
typedef struct {
  CAN_MotorModel_t model;
  uint8_t id;
  CAN_MotorFeedback_t feedback;
} CAN_Motor_t;

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

typedef union {
  float as_array[4];
  struct {
    float m1;
    float m2;
    float m3;
    float m4;
  } named;
} CAN_ChassisOutput_t;

typedef union {
  float as_array[3];
  AHRS_Eulr_t named;
} CAN_GimbalOutput_t;

typedef union {
  float as_array[3];
  struct {
    float fric1;
    float fric2;
    float trig;
  } named;
} CAN_ShootOutput_t;

typedef struct {
  float power_limit;
} CAN_CapOutput_t;

typedef union {
  CAN_ChassisOutput_t chassis;
  CAN_GimbalOutput_t gimbal;
  CAN_ShootOutput_t shoot;
  CAN_CapOutput_t cap;
} CAN_Output_t;

typedef enum {
  CAN_MOTOR_GROUT_CHASSIS = 0,
  CAN_MOTOR_GROUT_GIMBAL1,
  CAN_MOTOR_GROUT_GIMBAL2,
  CAN_MOTOR_GROUT_SHOOT1,
  CAN_MOTOR_GROUT_SHOOT2,
  CAN_MOTOR_GROUT_CAP,
  CAN_MOTOR_GROUT_NUM,
} CAN_MotorGroup_t;

// TODO: 不懂什么意思，计算电容剩余电量的话，用component/capacity
typedef enum {
  CAP_STATUS_OFFLINE,
  CAP_STATUS_CHARGING,
  CAP_STATUS_RUNNING1,
  CAP_STATUS_RUNNING2,
  CAP_STATUS_RUNNING3
} Cap_Status_t;

typedef union {
  CAN_MotorFeedback_t as_array[4];
  struct {
    CAN_MotorFeedback_t m1;
    CAN_MotorFeedback_t m2;
    CAN_MotorFeedback_t m3;
    CAN_MotorFeedback_t m4;
  } named;
} CAN_ChassisMotor_t;

typedef union {
  CAN_MotorFeedback_t as_array[2];
  struct {
    CAN_MotorFeedback_t yaw;
    CAN_MotorFeedback_t pit;
  } named;
} CAN_GimbalMotor_t;

typedef union {
  CAN_MotorFeedback_t as_array[3];
  struct {
    CAN_MotorFeedback_t fric1;
    CAN_MotorFeedback_t fric2;
    CAN_MotorFeedback_t trig;
  } named;
} CAN_ShootMotor_t;

typedef struct {
  float input_volt;
  float cap_volt;
  float input_curr;
  float target_power;
} CAN_CapFeedback_t;
// TODO: CAN_Capacitor_t

typedef struct {
  CAN_RxHeaderTypeDef rx_header;
  // TODO: 把这段计算放到文件前面多好，用预处理直接计算出来
  uint8_t rx_data[(CAN_MOTOR_TX_BUF_SIZE > CAN_CAP_TX_BUF_SIZE)
                      ? CAN_MOTOR_TX_BUF_SIZE
                      : CAN_CAP_TX_BUF_SIZE];
} CAN_RawRx_t;

typedef struct {
  CAN_TxHeaderTypeDef tx_header;
  // TODO: 把这段计算放到文件前面多好，用预处理直接计算出来
  uint8_t tx_data[(CAN_MOTOR_TX_BUF_SIZE > CAN_CAP_TX_BUF_SIZE)
                      ? CAN_MOTOR_TX_BUF_SIZE
                      : CAN_CAP_TX_BUF_SIZE];
} CAN_RawTx_t;

typedef struct {
  uint32_t motor_flag;

  CAN_ChassisMotor_t chassis_motor;
  CAN_GimbalMotor_t gimbal_motor;
  CAN_ShootMotor_t shoot_motor;
  // TODO: CAN_Capacitor_t cap
  Cap_Status_t cap_status;
  CAN_CapFeedback_t cap_feedback;
  osMessageQueueId_t msgq_raw_motor;
  osMessageQueueId_t msgq_raw_cap;
} CAN_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t CAN_Init(CAN_t *can);
CAN_t *CAN_GetDevice(void);

int8_t CAN_Motor_Control(CAN_MotorGroup_t group, CAN_Output_t *output);
int8_t CAN_Motor_StoreMsg(CAN_t *can, CAN_RawRx_t *can_motor_rx);
bool CAN_Motor_CheckFlag(CAN_t *can, uint32_t flag);
int8_t CAN_Motor_ClearFlag(CAN_t *can, uint32_t flag);

void CAN_ResetChassisOut(CAN_ChassisOutput_t *chassis_out);
void CAN_ResetGimbalOut(CAN_GimbalOutput_t *gimbal_out);
void CAN_ResetShootOut(CAN_ShootOutput_t *shoot_out);

  //TODO: CAN_Cap_Control 加个下划线区分Cap和motor，丑了点但是更清楚
int8_t CAN_CapControl(float power_limit);
void CAN_ResetCapOut(CAN_CapOutput_t *cap_out);
  //TODO: 要是和电机函数功能相似的话可以考虑叫CAN_Cap_StoreMsg
void CAN_Cap_Decode(CAN_CapFeedback_t *feedback, const uint8_t *raw);

#ifdef __cplusplus
}
#endif
