#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdbool.h>

#include "bsp/can.h"
#include "component/ahrs.h"
#include "component/user_math.h"
#include "device/device.h"

/* Exported constants ------------------------------------------------------- */
#define CAN_MOTOR_CHASSIS_1_RECV (1 << 0)
#define CAN_MOTOR_CHASSIS_2_RECV (1 << 1)
#define CAN_MOTOR_CHASSIS_3_RECV (1 << 2)
#define CAN_MOTOR_CHASSIS_4_RECV (1 << 3)
#define CAN_MOTOR_GIMBAL_YAW_RECV (1 << 4)
#define CAN_MOTOR_GIMBAL_PIT_RECV (1 << 5)
#define CAN_MOTOR_LAUNCHER_FRIC1_RECV (1 << 6)
#define CAN_MOTOR_LAUNCHER_FRIC2_RECV (1 << 7)
#define CAN_MOTOR_LAUNCHER_TRIG_RECV (1 << 8)
#define CAN_MOTOR_CAP_RECV (1 << 9)
#define CAN_TOF_RECV (1 << 10)

#define CAN_REC_CHASSIS_FINISHED                         \
  (CAN_MOTOR_CHASSIS_1_RECV | CAN_MOTOR_CHASSIS_2_RECV | \
   CAN_MOTOR_CHASSIS_3_RECV | CAN_MOTOR_CHASSIS_4_RECV)
#define CAN_REC_GIMBAL_FINISHED \
  (CAN_MOTOR_GIMBAL_YAW_RECV | CAN_MOTOR_GIMBAL_PIT_RECV)
#define CAN_REC_LAUNCHER_FINISHED                                  \
  (CAN_MOTOR_LAUNCHER_FRIC1_RECV | CAN_MOTOR_LAUNCHER_FRIC2_RECV | \
   CAN_MOTOR_LAUNCHER_TRIG_RECV)
#define CAN_REC_CAP_FINISHED CAN_MOTOR_CAP_RECV
#define CAN_REC_TOF_FINISHED CAN_TOF_RECV

#define CAN_MOTOR_TX_BUF_SIZE (8)
#define CAN_MOTOR_RX_BUF_SIZE (8)

#define CAN_CAP_TX_BUF_SIZE (8)
#define CAN_CAP_RX_BUF_SIZE (8)

#define CAN_TX_BUF_SIZE_MAX                                              \
  ((CAN_MOTOR_TX_BUF_SIZE > CAN_CAP_TX_BUF_SIZE) ? CAN_MOTOR_TX_BUF_SIZE \
                                                 : CAN_CAP_TX_BUF_SIZE)
#define CAN_RX_BUF_SIZE_MAX                                              \
  ((CAN_MOTOR_RX_BUF_SIZE > CAN_CAP_RX_BUF_SIZE) ? CAN_MOTOR_RX_BUF_SIZE \
                                                 : CAN_CAP_RX_BUF_SIZE)

/* 电机最大电流绝对值 */
#define CAN_GM6020_MAX_ABS_CUR (1)
#define CAN_M3508_MAX_ABS_CUR (20)
#define CAN_M2006_MAX_ABS_CUR (10)
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

typedef struct {
  BSP_CAN_t chassis;
  BSP_CAN_t gimbal;
  BSP_CAN_t launcher;
  BSP_CAN_t cap;
} CAN_Params_t;

/* 电机反馈信息 */
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

enum CAN_MotorLauncher_e {
  CAN_MOTOR_LAUNCHER_FRIC1 = 0,
  CAN_MOTOR_LAUNCHER_FRIC2,
  CAN_MOTOR_LAUNCHER_TRIG,
  CAN_MOTORLAUNCHER_NUM,
};

typedef struct {
  uint8_t num;
  uint32_t id_start;
} CAN_MotorGroupInit_t;

typedef struct {
  CAN_MotorGroupInit_t chassis;
  CAN_MotorGroupInit_t gimbal1;
  CAN_MotorGroupInit_t gimbal2;
  CAN_MotorGroupInit_t launcher1;
  CAN_MotorGroupInit_t launcher2;
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
} CAN_LauncherOutput_t;

typedef struct {
  float power_limit;
} CAN_CapOutput_t;

typedef struct {
  CAN_ChassisOutput_t chassis;
  CAN_GimbalOutput_t gimbal;
  CAN_LauncherOutput_t launcher;
  CAN_CapOutput_t cap;
} CAN_Output_t;

typedef enum {
  CAN_MOTOR_GROUT_CHASSIS = 0,
  CAN_MOTOR_GROUT_GIMBAL1,
  CAN_MOTOR_GROUT_GIMBAL2,
  CAN_MOTOR_GROUT_LAUNCHER1,
  CAN_MOTOR_GROUT_LAUNCHER2,
  CAN_MOTOR_GROUT_CAP,
  CAN_MOTOR_GROUT_NUM,
} CAN_MotorGroup_t;

typedef enum {
  CAN_CAP_STATUS_OFFLINE,
  CAN_CAP_STATUS_RUNNING,
} CAN_CapStatus_t;

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
} CAN_LauncherMotor_t;

typedef struct {
  float input_volt;
  float cap_volt;
  float input_curr;
  float target_power;
} CAN_CapFeedback_t;

typedef struct {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[CAN_RX_BUF_SIZE_MAX];
} CAN_RawRx_t;

typedef struct {
  CAN_TxHeaderTypeDef tx_header;
  uint8_t tx_data[CAN_TX_BUF_SIZE_MAX];
} CAN_RawTx_t;

typedef struct {
  float percentage;
  CAN_CapStatus_t cap_status;
  CAN_CapFeedback_t cap_feedback;
} CAN_Capacitor_t;

typedef struct {
  CAN_ChassisMotor_t chassis;
  CAN_GimbalMotor_t gimbal;
  CAN_LauncherMotor_t launcher;
} CAN_Motor_t;

typedef struct {
  float dist;
  uint8_t status;
  uint16_t signal_strength;
} CAN_Tof_t;

typedef struct {
  uint32_t recive_flag;

  CAN_Motor_t motor;
  CAN_Capacitor_t cap;
  CAN_Tof_t tof;
  const CAN_Params_t *param;
  struct {
    uint32_t chassis;
    uint32_t gimbal;
    uint32_t launcher;
    uint32_t cap;
  } mailbox;
  osMessageQueueId_t msgq_raw;
} CAN_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t CAN_Init(CAN_t *can, const CAN_Params_t *param);

int8_t CAN_Motor_Control(CAN_MotorGroup_t group, CAN_Output_t *output,
                         CAN_t *can);
int8_t CAN_StoreMsg(CAN_t *can, CAN_RawRx_t *can_rx);
bool CAN_CheckFlag(CAN_t *can, uint32_t flag, bool clear_on_hit);
int8_t CAN_ClearFlag(CAN_t *can, uint32_t flag);

int8_t CAN_Cap_Control(CAN_CapOutput_t *output, CAN_t *can);
void CAN_ResetCapOut(CAN_CapOutput_t *cap_out);
void CAN_Cap_Decode(CAN_CapFeedback_t *feedback, const uint8_t *raw);

void CAN_Tof_Decode(CAN_Tof_t *tof, const uint8_t *raw);

#ifdef __cplusplus
}
#endif
