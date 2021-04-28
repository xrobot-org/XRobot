/*
  CAN总线上的设
  将所有CAN总线上挂载的设抽象成一设进行配和控制
*/

/* Includes ----------------------------------------------------------------- */
#include "can.h"

#include <stdbool.h>
#include <string.h>

#include "bsp/can.h"
#include "bsp/mm.h"
#include "component/utils.h"
#include "device/referee.h"

/* Private define ----------------------------------------------------------- */
/* Motor id */
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

/* 电机最大控制输出绝对值 */
#define CAN_GM6020_MAX_ABS_LSB (30000)
#define CAN_M3508_MAX_ABS_LSB (16384)
#define CAN_M2006_MAX_ABS_LSB (10000)

#define CAN_MOTOR_TX_BUF_SIZE (8)
#define CAN_MOTOR_RX_BUF_SIZE (8)

#define CAN_MOTOR_ENC_RES (8192)  /* 电机编码器分辨率 */
#define CAN_MOTOR_CUR_RES (16384) /* 电机转矩电流分辨率 */

/* Super capacitor */
#define CAN_CAP_FB_ID_BASE (0x211)
#define CAN_CAP_CTRL_ID_BASE (0x210)

#define CAN_CAP_RES (100) /* 电容数据分辨率 */

/* TOF */
#define CAN_TOF_ID_BASE (0x280)

#define CAN1_RX_FIFO CAN_RX_FIFO0
#define CAN2_RX_FIFO CAN_RX_FIFO1

/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static CAN_RawRx_t raw_rx1, raw_rx2;
static CAN_RawTx_t raw_tx;

static osThreadId_t thread_alert;
static CAN_t *gcan;
static bool inited = false;

/* Private function  -------------------------------------------------------- */
static void CAN_Motor_Decode(CAN_MotorFeedback_t *feedback,
                             const uint8_t *raw) {
  uint16_t raw_angle = (uint16_t)((raw[0] << 8) | raw[1]);
  int16_t raw_current = (int16_t)((raw[4] << 8) | raw[5]);

  feedback->rotor_angle = raw_angle / (float)CAN_MOTOR_ENC_RES * M_2PI;
  feedback->rotor_speed = (int16_t)((raw[2] << 8) | raw[3]);
  feedback->torque_current =
      raw_current * CAN_M3508_MAX_ABS_CUR / (float)CAN_MOTOR_CUR_RES;
  feedback->temp = raw[6];
}

void CAN_Cap_Decode(CAN_CapFeedback_t *feedback, const uint8_t *raw) {
  feedback->input_volt = (float)((raw[1] << 8) | raw[0]) / (float)CAN_CAP_RES;
  feedback->cap_volt = (float)((raw[3] << 8) | raw[2]) / (float)CAN_CAP_RES;
  feedback->input_curr = (float)((raw[5] << 8) | raw[4]) / (float)CAN_CAP_RES;
  feedback->target_power = (float)((raw[7] << 8) | raw[6]) / (float)CAN_CAP_RES;
}

void CAN_Tof_Decode(CAN_Tof_t *tof, const uint8_t *raw) {
  tof->dist = (float)((raw[2] << 16) | (raw[1] << 8) | raw[0]) / 1000.0f;
  tof->status = raw[3];
  tof->signal_strength = (raw[5] << 8) | raw[4];
}

static void CAN_CAN1RxFifoMsgPendingCallback(void) {
  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_1), CAN1_RX_FIFO,
                       &raw_rx1.rx_header, raw_rx1.rx_data);
  osMessageQueuePut(gcan->msgq_raw, &raw_rx1, 0, 0);
}

static void CAN_CAN2RxFifoMsgPendingCallback(void) {
  HAL_CAN_GetRxMessage(BSP_CAN_GetHandle(BSP_CAN_2), CAN2_RX_FIFO,
                       &raw_rx2.rx_header, raw_rx2.rx_data);
  osMessageQueuePut(gcan->msgq_raw, &raw_rx2, 0, 0);
}

/* Exported functions ------------------------------------------------------- */
int8_t CAN_Init(CAN_t *can, const CAN_Params_t *param) {
  ASSERT(can);
  if (inited) return DEVICE_ERR_INITED;
  VERIFY((thread_alert = osThreadGetId()) != NULL);

  can->msgq_raw = osMessageQueueNew(32, sizeof(CAN_RawRx_t), NULL);

  can->param = param;

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
  can_filter.FilterFIFOAssignment = CAN1_RX_FIFO;

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_1), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_1));
  BSP_CAN_RegisterCallback(BSP_CAN_1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
                           CAN_CAN1RxFifoMsgPendingCallback);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_1),
                               CAN_IT_RX_FIFO0_MSG_PENDING);

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN2_RX_FIFO;

  HAL_CAN_ConfigFilter(BSP_CAN_GetHandle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(BSP_CAN_GetHandle(BSP_CAN_2));
  BSP_CAN_RegisterCallback(BSP_CAN_2, HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
                           CAN_CAN2RxFifoMsgPendingCallback);
  HAL_CAN_ActivateNotification(BSP_CAN_GetHandle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);

  gcan = can;
  inited = true;
  return DEVICE_OK;
}

int8_t CAN_Motor_Control(CAN_MotorGroup_t group, CAN_Output_t *output,
                         CAN_t *can) {
  ASSERT(output);
  ASSERT(can);

  int16_t motor1, motor2, motor3, motor4;
  int16_t yaw_motor, pit_motor;
  int16_t fric1_motor, fric2_motor, trig_motor;
  switch (group) {
    case CAN_MOTOR_GROUT_CHASSIS:
      motor1 =
          (int16_t)(output->chassis.named.m1 * (float)CAN_M3508_MAX_ABS_LSB);
      motor2 =
          (int16_t)(output->chassis.named.m2 * (float)CAN_M3508_MAX_ABS_LSB);
      motor3 =
          (int16_t)(output->chassis.named.m3 * (float)CAN_M3508_MAX_ABS_LSB);
      motor4 =
          (int16_t)(output->chassis.named.m4 * (float)CAN_M3508_MAX_ABS_LSB);

      raw_tx.tx_header.StdId = CAN_M3508_M2006_CTRL_ID_BASE;
      raw_tx.tx_header.IDE = CAN_ID_STD;
      raw_tx.tx_header.RTR = CAN_RTR_DATA;
      raw_tx.tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

      raw_tx.tx_data[0] = (uint8_t)((motor1 >> 8) & 0xFF);
      raw_tx.tx_data[1] = (uint8_t)(motor1 & 0xFF);
      raw_tx.tx_data[2] = (uint8_t)((motor2 >> 8) & 0xFF);
      raw_tx.tx_data[3] = (uint8_t)(motor2 & 0xFF);
      raw_tx.tx_data[4] = (uint8_t)((motor3 >> 8) & 0xFF);
      raw_tx.tx_data[5] = (uint8_t)(motor3 & 0xFF);
      raw_tx.tx_data[6] = (uint8_t)((motor4 >> 8) & 0xFF);
      raw_tx.tx_data[7] = (uint8_t)(motor4 & 0xFF);

      HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(can->param->chassis),
                           &raw_tx.tx_header, raw_tx.tx_data,
                           &(can->mailbox.chassis));
      break;
    case CAN_MOTOR_GROUT_GIMBAL1:
    case CAN_MOTOR_GROUT_GIMBAL2:
      yaw_motor =
          (int16_t)(output->gimbal.named.yaw * (float)CAN_GM6020_MAX_ABS_LSB);
      pit_motor =
          (int16_t)(output->gimbal.named.pit * (float)CAN_GM6020_MAX_ABS_LSB);
      raw_tx.tx_header.StdId = CAN_GM6020_CTRL_ID_EXTAND;
      raw_tx.tx_header.IDE = CAN_ID_STD;
      raw_tx.tx_header.RTR = CAN_RTR_DATA;
      raw_tx.tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

      raw_tx.tx_data[0] = (uint8_t)((yaw_motor >> 8) & 0xFF);
      raw_tx.tx_data[1] = (uint8_t)(yaw_motor & 0xFF);
      raw_tx.tx_data[2] = (uint8_t)((pit_motor >> 8) & 0xFF);
      raw_tx.tx_data[3] = (uint8_t)(pit_motor & 0xFF);
      raw_tx.tx_data[4] = 0;
      raw_tx.tx_data[5] = 0;
      raw_tx.tx_data[6] = 0;
      raw_tx.tx_data[7] = 0;

      HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(can->param->gimbal),
                           &raw_tx.tx_header, raw_tx.tx_data,
                           &(can->mailbox.gimbal));
      break;

    case CAN_MOTOR_GROUT_LAUNCHER1:
    case CAN_MOTOR_GROUT_LAUNCHER2:
      fric1_motor = (int16_t)(output->launcher.named.fric1 *
                              (float)CAN_M3508_MAX_ABS_LSB);
      fric2_motor = (int16_t)(output->launcher.named.fric2 *
                              (float)CAN_M3508_MAX_ABS_LSB);
      trig_motor =
          (int16_t)(output->launcher.named.trig * (float)CAN_M2006_MAX_ABS_LSB);

      raw_tx.tx_header.StdId = CAN_M3508_M2006_CTRL_ID_EXTAND;
      raw_tx.tx_header.IDE = CAN_ID_STD;
      raw_tx.tx_header.RTR = CAN_RTR_DATA;
      raw_tx.tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

      raw_tx.tx_data[0] = (uint8_t)((fric1_motor >> 8) & 0xFF);
      raw_tx.tx_data[1] = (uint8_t)(fric1_motor & 0xFF);
      raw_tx.tx_data[2] = (uint8_t)((fric2_motor >> 8) & 0xFF);
      raw_tx.tx_data[3] = (uint8_t)(fric2_motor & 0xFF);
      raw_tx.tx_data[4] = (uint8_t)((trig_motor >> 8) & 0xFF);
      raw_tx.tx_data[5] = (uint8_t)(trig_motor & 0xFF);
      raw_tx.tx_data[6] = 0;
      raw_tx.tx_data[7] = 0;

      HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(can->param->launcher),
                           &raw_tx.tx_header, raw_tx.tx_data,
                           &(can->mailbox.launcher));
      break;

    default:
      break;
  }
  return DEVICE_OK;
}

int8_t CAN_StoreMsg(CAN_t *can, CAN_RawRx_t *can_rx) {
  ASSERT(can);
  ASSERT(can_rx);

  uint32_t index;
  switch (can_rx->rx_header.StdId) {
    case CAN_M3508_M1_ID:
    case CAN_M3508_M2_ID:
    case CAN_M3508_M3_ID:
    case CAN_M3508_M4_ID:
      index = can_rx->rx_header.StdId - CAN_M3508_M1_ID;
      CAN_Motor_Decode(can->motor.chassis.as_array + index, can_rx->rx_data);
      can->recive_flag |= 1 << index;
      break;

    case CAN_M3508_FRIC1_ID:
    case CAN_M3508_FRIC2_ID:
    case CAN_M2006_TRIG_ID:
      index = can_rx->rx_header.StdId - CAN_M3508_FRIC1_ID;
      CAN_Motor_Decode(can->motor.launcher.as_array + index, can_rx->rx_data);
      can->recive_flag |= 1 << (index + 6);
      break;

    case CAN_GM6020_YAW_ID:
    case CAN_GM6020_PIT_ID:
      index = can_rx->rx_header.StdId - CAN_GM6020_YAW_ID;
      CAN_Motor_Decode(can->motor.gimbal.as_array + index, can_rx->rx_data);
      can->recive_flag |= 1 << (index + 4);
      break;
    case CAN_CAP_FB_ID_BASE:
      CAN_Cap_Decode(&(can->cap.cap_feedback), can_rx->rx_data);
      can->recive_flag |= 1 << 9;
      break;
    case CAN_TOF_ID_BASE:
      CAN_Tof_Decode(&(can->tof), can_rx->rx_data);
      can->recive_flag |= 1 << 10;
      break;
    default:
      break;
  }
  return DEVICE_OK;
}

bool CAN_CheckFlag(CAN_t *can, uint32_t flag, bool clear_on_hit) {
  ASSERT(can);
  bool pass = (can->recive_flag & flag) == flag;
  if (clear_on_hit && pass) CAN_ClearFlag(can, flag);
  return pass;
}

int8_t CAN_ClearFlag(CAN_t *can, uint32_t flag) {
  ASSERT(can);
  can->recive_flag &= ~flag;
  return DEVICE_OK;
}

int8_t CAN_Cap_Control(CAN_CapOutput_t *output, CAN_t *can) {
  float power_limit = output->power_limit;

  uint16_t cap = (uint16_t)(power_limit * CAN_CAP_RES);

  raw_tx.tx_header.StdId = CAN_CAP_CTRL_ID_BASE;
  raw_tx.tx_header.IDE = CAN_ID_STD;
  raw_tx.tx_header.RTR = CAN_RTR_DATA;
  raw_tx.tx_header.DLC = CAN_MOTOR_TX_BUF_SIZE;

  raw_tx.tx_data[0] = (cap >> 8) & 0xFF;
  raw_tx.tx_data[1] = cap & 0xFF;

  HAL_CAN_AddTxMessage(BSP_CAN_GetHandle(can->param->cap), &raw_tx.tx_header,
                       raw_tx.tx_data, &(can->mailbox.cap));
  return DEVICE_OK;
}
