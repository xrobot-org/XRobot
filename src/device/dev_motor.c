#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"
#include "dev_motor.h"

/* 电机最大控制输出绝对值 */
#define GM6020_MAX_ABS_LSB (30000)
#define M3508_MAX_ABS_LSB (16384)
#define M2006_MAX_ABS_LSB (10000)

/* 电机最大电流绝对值 */
#define GM6020_MAX_ABS_CUR (1)
#define M3508_MAX_ABS_CUR (20)
#define M2006_MAX_ABS_CUR (10)

#define MOTOR_ENC_RES (8192)  /* 电机编码器分辨率 */
#define MOTOR_CUR_RES (16384) /* 电机转矩电流分辨率 */

static bool inited = false;

static uint32_t motor_ctrl_id_map[] = {M3508_M2006_CTRL_ID_BASE,
                                       M3508_M2006_CTRL_ID_EXTAND,
                                       GM6020_CTRL_ID_EXTAND};

#define MOTOR_CTRL_ID_NUMBER (sizeof(motor_ctrl_id_map) / sizeof(uint32_t))

static uint8_t motor_tx_buff[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER][CAN_DATA_SIZE];
static bool motor_tx_map[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

//   M3508_M1_ID = 0x201, /* 1 */
//   M3508_M2_ID = 0x202, /* 2 */
//   M3508_M3_ID = 0x203, /* 3 */
//   M3508_M4_ID = 0x204, /* 4 */

//   M3508_FRIC1_ID = 0x205, /* 5 */
//   M3508_FRIC2_ID = 0x206, /* 6 */
//   M2006_TRIG_ID = 0x207,  /* 7 */

//   GM6020_YAW_ID = 0x209, /* 5 */
//   GM6020_PIT_ID = 0x20A, /* 6 */

static float Motor_ModelToLSB(motor_model_t model) {
  switch (model) {
    case MOTOR_M2006:
      return (float)M2006_MAX_ABS_LSB;

    case MOTOR_M3508:
      return (float)M3508_MAX_ABS_LSB;

    case MOTOR_GM6020:
      return (float)GM6020_MAX_ABS_LSB;

    default:
      return 0.f;
  }
}

static uint32_t motor_get_tx_data_offset(motor_model_t model,
                                         uint32_t feedback_id,
                                         uint32_t control_id) {
  switch (model) {
    case MOTOR_M3508:
    case MOTOR_M2006:
      switch (control_id) {
        case M3508_M2006_CTRL_ID_BASE:
          return feedback_id - M3508_M2006_FB_ID_BASE;

        case M3508_M2006_CTRL_ID_EXTAND:
          return feedback_id - M3508_M2006_FB_ID_EXTAND;

        default:
          return 0;
      }
    case MOTOR_GM6020:
      switch (control_id) {
        case GM6020_CTRL_ID_BASE:
          return feedback_id - GM6020_FB_ID_BASE;
        case GM6020_CTRL_ID_EXTAND:
          return feedback_id - GM6020_FB_ID_EXTAND;

        default:
          return 0;
      }
    default:
      return 0;
  }
}

static err_t Motor_Decode(motor_feedback_t *fb, const uint8_t *raw) {
  ASSERT(fb);
  ASSERT(raw);

  uint16_t raw_angle = (uint16_t)((raw[0] << 8) | raw[1]);
  int16_t raw_current = (int16_t)((raw[4] << 8) | raw[5]);

  fb->rotor_abs_angle = raw_angle / (float)MOTOR_ENC_RES * M_2PI;
  fb->rotational_speed = (int16_t)((raw[2] << 8) | raw[3]);
  fb->torque_current = raw_current * M3508_MAX_ABS_CUR / (float)MOTOR_CUR_RES;
  fb->temp = raw[6];
  return RM_OK;
}

static void motor_rx_callback(can_rx_item_t *rx, void *arg) {
  ASSERT(rx);
  ASSERT(arg);
  QueueHandle_t msgq = (QueueHandle_t)arg;

  BaseType_t switch_required;
  xQueueOverwriteFromISR(msgq, rx, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

err_t motor_init(motor_t *motor, const motor_group_t *group_cfg) {
  ASSERT(motor);
  ASSERT(group_cfg);

  if (inited) return DEVICE_ERR_INITED;
  motor->group_cfg = group_cfg;

  const motor_group_t *group = motor->group_cfg;
  for (int i = 0; i < MOTOR_GROUP_ID_NUM; i++) {
    motor->msgq[i] = xQueueCreate(1, sizeof(can_rx_item_t));
    BSP_CAN_RegisterSubscriber(group->can, group->id_feedback, group->num,
                               motor_rx_callback, motor->msgq[i]);
    group++;
  }

  inited = true;
  return DEVICE_OK;
}

err_t motor_update(motor_t *motor, uint32_t timeout) {
  can_rx_item_t pack;

  for (int i = 0; i < MOTOR_GROUP_ID_NUM; i++) {
    while (pdPASS ==
           xQueueReceive(motor->msgq[i], &pack, pdMS_TO_TICKS(timeout))) {
      if ((pack.index < motor->group_cfg[i].num) &&
          (MOTOR_NONE != motor->group_cfg[i].model[pack.index]))
        Motor_Decode(&(motor->feedback[i].as_array[pack.index]), pack.data);
      break;
    }
  }

  return RM_OK;
}

err_t motor_pack_data(motor_t *motor, motor_group_id_t group,
                      motor_control_t *output) {
  ASSERT(output);
  ASSERT(motor);

  uint8_t *data = NULL;

  for (uint32_t i = 0; i < MOTOR_CTRL_ID_NUMBER; i++) {
    if (motor->group_cfg[group].id_control == motor_ctrl_id_map[i]) {
      motor_tx_map[motor->group_cfg[group].can][i] = true;
      data = motor_tx_buff[motor->group_cfg[group].can][i];
      break;
    }
  }

  int16_t motor_data;

  uint32_t id_offset = motor_get_tx_data_offset(
      *motor->group_cfg[group].model, motor->group_cfg[group].id_feedback,
      motor->group_cfg[group].id_control);

  for (size_t i = 0; i < motor->group_cfg[group].num; i++) {
    float lsb = Motor_ModelToLSB(motor->group_cfg[group].model[i]);

    if (lsb) {
      motor_data = (int16_t)(output->as_array[i] * lsb);
      data[2 * (i + id_offset)] = (uint8_t)((motor_data >> 8) & 0xFF);
      data[2 * (i + id_offset) + 1] = (uint8_t)(motor_data & 0xFF);
    }
  }
  return RM_OK;
}

err_t motor_control(motor_t *motor) {
  for (uint32_t i = 0; i < BSP_CAN_NUM; i++) {
    for (uint32_t t = 0; t < MOTOR_CTRL_ID_NUMBER; t++) {
      if (motor_tx_map[i][t])
        can_trans_packet(i, motor_ctrl_id_map[t], motor_tx_buff[i][t],
                         &motor->mailbox);
    }
  }

  memset(&motor_tx_map, 0, sizeof(motor_tx_map));

  return RM_OK;
}

err_t motor_handle_offline(motor_t *motor) {
  RM_UNUSED(motor);
  memset(motor, 0, sizeof(motor));
  return RM_OK;
}
