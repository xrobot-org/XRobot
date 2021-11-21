#include "dev_motor.h"

#include <stdbool.h>
#include <string.h>

#include "bsp_can.h"
#include "comp_utils.h"

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

err_t motor_init(motor_t *motor, const motor_group_t *group_cfg) {
  ASSERT(motor);
  ASSERT(group_cfg);

  if (inited) return DEVICE_ERR_INITED;
  motor->group_cfg = group_cfg;

  BSP_CAN_RegisterRxGroup();

  inited = true;
  return DEVICE_OK;
}

err_t motor_update(motor_t *motor, uint32_t timeout) {
  BSP_CAN_RxItem_t pack;
  while (pdPASS ==
         xQueueReceive(motor->msgq_rx, &pack, pdMS_TO_TICKS(timeout))) {
    for (size_t i = 0; i < MOTOR_GROUT_ID_NUM; i++) {
      size_t index = pack.id - motor->group_cfg[i].id_feedback;
      if ((index < motor->group_cfg[i].num) &&
          (MOTOR_NONE != motor->group_cfg[i].model[index])) {
        Motor_Decode(&(motor->feedback[i].as_array[index]), pack.data);
        break;
      } else {
        return ERR_PARAM;
      }
    }
  }
  return RM_OK;
}

err_t motor_control(motor_t *motor, motor_group_id_t group,
                    motor_control_t *output) {
  ASSERT(output);
  ASSERT(motor);

  BSP_CAN_RxItem_t pack = {0};
  int16_t data;

  for (size_t i = 0; i < ARRAY_LEN(output->as_array); i++) {
    float lsb = Motor_ModelToLSB(motor->group_cfg[group].model[i]);
    data = (int16_t)(output->as_array[i] * lsb);

    pack.data[2 * i] = (uint8_t)((data >> 8) & 0xFF);
    pack.data[2 * i + 1] = (uint8_t)(data & 0xFF);
  }
  pack.id = motor->group_cfg[group].id_control;

  xQueueSendToBack(motor->msgq_tx, &pack, 0);
  return RM_OK;
}
