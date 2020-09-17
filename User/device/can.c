/*
        CAN总线上的设
        将所有CAN总线上挂载的设抽象成一设进行配和控制
*/

/* Includes ----------------------------------------------------------------- */
#include "can.h"

#include <stdbool.h>
#include <string.h>

#include "bsp\can.h"
#include "bsp\mm.h"
#include "component\user_math.h"

/* Private define ----------------------------------------------------------- */
/* Motor */
/* id		feedback id		control id */
/* 1-4		0x205 to 0x208  0x1ff */
/* 5-6		0x209 to 0x20B  0x2ff */
#define CAN_GM6020_FB_ID_BASE (0x205)
#define CAN_GM6020_CTRL_ID_BASE (0x1ff)
#define CAN_GM6020_CTRL_ID_EXTAND (0x2ff)

/* id		feedback id		control id */
/* 1-4		0x201 to 0x204  0x200 */
/* 5-6		0x205 to 0x208  0x1ff */
#define CAN_M3508_M2006_FB_ID_BASE (0x201)
#define CAN_M3508_M2006_CTRL_ID_BASE (0x200)
#define CAN_M3508_M2006_CTRL_ID_EXTAND (0x1ff)
#define CAN_M3508_M2006_ID_SETTING_ID (0x700)

#define CAN_GM6020_MAX_ABS_VOLT (30000)
#define CAN_M3508_MAX_ABS_VOLT (16384)
#define CAN_M2006_MAX_ABS_VOLT (10000)

#define CAN_MOTOR_MAX_NUM (9)

#define CAN_MOTOR_TX_BUF_SIZE (8)
#define CAN_MOTOR_RX_BUF_SIZE (8)

#define CAN_MOTOR_MAX_ENCODER (8191)
#define CAN_MOTOR_CAN_RX_FIFO CAN_RX_FIFO0

/* UWB */
#define CAN_UWB_FB_ID_BASE (0x259)
#define CAN_UWB_RX_BUF_SIZE (22)
#define CAN_UWB_TX_BUF_SIZE (22)

#define CAN_UWB_MAX_YAW (36000)
#define CAN_UWB_CAN_RX_FIFO CAN_RX_FIFO1

/* Super capacitor */
#define CAN_CAP_FB_ID_BASE (0x000)
#define CAN_CAP_CTRL_ID_BASE (0x000)

/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
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

/* Private variables -------------------------------------------------------- */
static uint8_t motor_received = 0;
static uint32_t unknown_message = 0;

static CAN_TxHeaderTypeDef tx_header;
static CAN_RxHeaderTypeDef rx_header;
static uint8_t motor_rx_data[CAN_MOTOR_TX_BUF_SIZE];
static uint8_t motor_tx_data[CAN_MOTOR_RX_BUF_SIZE];
static uint8_t uwb_rx_data[CAN_UWB_RX_BUF_SIZE];

static const CAN_MotorInit_t default_motor_init = {
    .chassis =
        {
            .num = 4,
            .id_start = 1,
        },

    .gimbal1 =
        {
            .num = 2,
            .id_start = 5,
        },

    .shoot1 =
        {
            .num = 3,
            .id_start = 5,
        },
};

static CAN_t *gcan;
static bool inited = false;

/* Private function  -------------------------------------------------------- */
static void CAN_Motor_Decode(CAN_MotorFeedback_t *feedback, const uint8_t *raw) {
  uint16_t raw_angle = (uint16_t)((raw[0] << 8) | raw[1]);

  feedback->rotor_angle = raw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * M_PI;
  feedback->rotor_speed = (int16_t)((raw[2] << 8) | raw[3]);
  feedback->torque_current = (int16_t)((raw[4] << 8) | raw[5]);
  feedback->temp = raw[6];

  motor_received++;
}

static void CAN_UWB_Decode(CAN_UWBFeedback_t *feedback, const uint8_t *raw) {
  memcmp(&(feedback->data), raw, 8);
}

static void CAN_Cap_Decode(CAN_CapFeedback_t *feedback, const uint8_t *raw) {
  // TODO
  (void)feedback;
  (void)raw;
}

bool CAN_CheckMotorInit(const CAN_MotorInit_t *mi) { return true; }

static void CAN_RxFifo0MsgPendingCallback(void) {
  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_1), CAN_MOTOR_CAN_RX_FIFO,
                       &rx_header, motor_rx_data);

  uint32_t index;
  switch (rx_header.StdId) {
    case CAN_M3508_M1_ID:
    case CAN_M3508_M2_ID:
    case CAN_M3508_M3_ID:
    case CAN_M3508_M4_ID:
      index = rx_header.StdId - CAN_M3508_M1_ID;
      CAN_Motor_Decode(&(gcan->chassis_motor_feedback[index]), motor_rx_data);
      break;

    case CAN_M3508_FRIC1_ID:
    case CAN_M3508_FRIC2_ID:
    case CAN_M2006_TRIG_ID:
      index = rx_header.StdId - CAN_M3508_FRIC1_ID;
      CAN_Motor_Decode(&(gcan->shoot_motor_feedback[index]), motor_rx_data);
      break;

    case CAN_GM6020_YAW_ID:
    case CAN_GM6020_PIT_ID:
      index = rx_header.StdId - CAN_GM6020_YAW_ID;
      CAN_Motor_Decode(&(gcan->gimbal_motor_feedback[index]), motor_rx_data);
      break;

    case CAN_CAP_FB_ID_BASE:
      CAN_Cap_Decode(&(gcan->cap_feedback), motor_rx_data);
      osThreadFlagsSet(gcan->cap_alert, SIGNAL_CAN_CAP_RECV);
      break;

    default:
      unknown_message++;
      break;
  }

  for (uint8_t i = 0; i < gcan->motor_alert_len; i++) {
    if (gcan->motor_alert[i]) {
      osThreadFlagsSet(gcan->motor_alert[i], SIGNAL_CAN_MOTOR_RECV);
    }
  }
}

static void CAN_RxFifo1MsgPendingCallback(void) {
  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_1), CAN_UWB_CAN_RX_FIFO,
                       &rx_header, uwb_rx_data);

  switch (rx_header.StdId) {
    case CAN_UWB_FB_ID_BASE:
      CAN_UWB_Decode(&(gcan->uwb_feedback), uwb_rx_data);
      osThreadFlagsSet(gcan->uwb_alert, SIGNAL_CAN_UWB_RECV);
      break;

    default:
      unknown_message++;
      break;
  }
}

/* Exported functions ------------------------------------------------------- */
int8_t CAN_Init(CAN_t *can, CAN_MotorInit_t *motor_init,
                osThreadId_t *motor_alert, uint8_t motor_alert_len,
                osThreadId_t uwb_alert, osThreadId_t cap_alert) {
  if (can == NULL) return DEVICE_ERR_NULL;

  if (motor_alert == NULL) return DEVICE_ERR_NULL;

  if (inited) return DEVICE_ERR_INITED;

  can->motor_alert_len = motor_alert_len;
  can->motor_alert = motor_alert;
  can->uwb_alert = uwb_alert;
  can->cap_alert = cap_alert;

  CAN_CheckMotorInit(motor_init);

  CAN_FilterTypeDef can_filter = {0};

  can_filter.FilterBank = 0;
  can_filter.FilterIdHigh = 0;
  can_filter.FilterIdLow = 0;
  can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
  can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
  can_filter.FilterMaskIdHigh = 0;
  can_filter.FilterMaskIdLow = 0;
  can_filter.FilterActivation = ENABLE;
  can_filter.SlaveStartFilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_MOTOR_CAN_RX_FIFO;

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_1), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_1));
  BSP_CAN_RegisterCallback(BSP_CAN_1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
                           CAN_RxFifo0MsgPendingCallback);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_1),
                               CAN_IT_RX_FIFO0_MSG_PENDING);

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_UWB_CAN_RX_FIFO;

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_2));
  BSP_CAN_RegisterCallback(BSP_CAN_2, HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
                           CAN_RxFifo1MsgPendingCallback);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);

  gcan = can;
  inited = true;
  return DEVICE_OK;
}

CAN_t *CAN_GetDevice(void) {
  if (inited) {
    return gcan;
  }
  return NULL;
}

int8_t CAN_Motor_Control(CAN_MotorGroup_t group, const float *out,
                         uint8_t num) {
  float scaller;
  int16_t *motors;
  motors = BSP_Malloc(sizeof(*motors) * num);

  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  switch (group) {
    case CAN_MOTOR_GROUT_CHASSIS:
      scaller = (float)CAN_M3508_MAX_ABS_VOLT;
      tx_header.StdId = CAN_M3508_M2006_CTRL_ID_BASE;
      break;
    case CAN_MOTOR_GROUT_GIMBAL1:
    case CAN_MOTOR_GROUT_GIMBAL2:
      scaller = (float)CAN_M3508_MAX_ABS_VOLT;
      tx_header.StdId = CAN_GM6020_CTRL_ID_BASE;
      break;
    case CAN_MOTOR_GROUT_SHOOT1:
    case CAN_MOTOR_GROUT_SHOOT2:
      // TODO
      scaller = (float)CAN_M3508_MAX_ABS_VOLT;
      tx_header.StdId = CAN_M3508_M2006_CTRL_ID_EXTAND;
      break;
    default:
      break;
  }

  for (uint8_t i = 0; i < num; i++) {
    motors[i] = (int16_t)(out[i] * scaller);
  }
  for (uint8_t i = 0; i < num; i++) {
    motor_tx_data[2 * i] = (uint8_t)((motors[i] >> 8) & 0xFF);
    motor_tx_data[2 * i + 1] = (uint8_t)(motors[i] & 0xFF);
  }
  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(BSP_CAN_1), &tx_header, motor_tx_data,
                       (uint32_t *)CAN_TX_MAILBOX0);

  return DEVICE_OK;
}

int8_t CAN_Motor_ControlChassis(float m1, float m2, float m3, float m4) {
  int16_t motor1 = (int16_t)(m1 * (float)CAN_M3508_MAX_ABS_VOLT);
  int16_t motor2 = (int16_t)(m2 * (float)CAN_M3508_MAX_ABS_VOLT);
  int16_t motor3 = (int16_t)(m3 * (float)CAN_M3508_MAX_ABS_VOLT);
  int16_t motor4 = (int16_t)(m4 * (float)CAN_M3508_MAX_ABS_VOLT);

  tx_header.StdId = CAN_M3508_M2006_CTRL_ID_BASE;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  motor_tx_data[0] = (uint8_t)((motor1 >> 8) & 0xFF);
  motor_tx_data[1] = (uint8_t)(motor1 & 0xFF);
  motor_tx_data[2] = (uint8_t)((motor2 >> 8) & 0xFF);
  motor_tx_data[3] = (uint8_t)(motor2 & 0xFF);
  motor_tx_data[4] = (uint8_t)((motor3 >> 8) & 0xFF);
  motor_tx_data[5] = (uint8_t)(motor3 & 0xFF);
  motor_tx_data[6] = (uint8_t)((motor4 >> 8) & 0xFF);
  motor_tx_data[7] = (uint8_t)(motor4 & 0xFF);

  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(BSP_CAN_1), &tx_header, motor_tx_data,
                       (uint32_t *)CAN_TX_MAILBOX0);

  return DEVICE_OK;
}

int8_t CAN_Motor_ControlGimbal(float yaw, float pitch) {
  int16_t yaw_motor = (int16_t)(yaw * (float)CAN_GM6020_MAX_ABS_VOLT);
  int16_t pit_motor = (int16_t)(pitch * (float)CAN_GM6020_MAX_ABS_VOLT);

  tx_header.StdId = CAN_GM6020_CTRL_ID_EXTAND;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  motor_tx_data[0] = (uint8_t)((yaw_motor >> 8) & 0xFF);
  motor_tx_data[1] = (uint8_t)(yaw_motor & 0xFF);
  motor_tx_data[2] = (uint8_t)((pit_motor >> 8) & 0xFF);
  motor_tx_data[3] = (uint8_t)(pit_motor & 0xFF);
  motor_tx_data[4] = 0;
  motor_tx_data[5] = 0;
  motor_tx_data[6] = 0;
  motor_tx_data[7] = 0;

  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(BSP_CAN_1), &tx_header, motor_tx_data,
                       (uint32_t *)CAN_TX_MAILBOX1);

  return DEVICE_OK;
}

int8_t CAN_Motor_ControlShoot(float fric1, float fric2, float trig) {
  int16_t fric1_motor = (int16_t)(fric1 * (float)CAN_M3508_MAX_ABS_VOLT);
  int16_t fric2_motor = (int16_t)(fric2 * (float)CAN_M3508_MAX_ABS_VOLT);
  int16_t trig_motor = (int16_t)(trig * (float)CAN_M2006_MAX_ABS_VOLT);

  tx_header.StdId = CAN_M3508_M2006_CTRL_ID_EXTAND;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  motor_tx_data[0] = (uint8_t)((fric1_motor >> 8) & 0xFF);
  motor_tx_data[1] = (uint8_t)(fric1_motor & 0xFF);
  motor_tx_data[2] = (uint8_t)((fric2_motor >> 8) & 0xFF);
  motor_tx_data[3] = (uint8_t)(fric2_motor & 0xFF);
  motor_tx_data[4] = (uint8_t)((trig_motor >> 8) & 0xFF);
  motor_tx_data[5] = (uint8_t)(trig_motor & 0xFF);
  motor_tx_data[6] = 0;
  motor_tx_data[7] = 0;

  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(BSP_CAN_1), &tx_header, motor_tx_data,
                       (uint32_t *)CAN_TX_MAILBOX2);

  return DEVICE_OK;
}

int8_t CAN_Motor_QuickIdSetMode(void) {
  tx_header.StdId = CAN_M3508_M2006_ID_SETTING_ID;
  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(BSP_CAN_1), &tx_header, motor_tx_data,
                       (uint32_t *)CAN_TX_MAILBOX0);
  return DEVICE_OK;
}

int8_t CAN_CapControl(float power_limit) {
  (void)power_limit;
  // TODO
  return DEVICE_OK;
}