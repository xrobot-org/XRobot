#include "dev_motor.hpp"

#include <string.h>

#include "comp_utils.hpp"

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

using namespace Device;

uint8_t Motor::motor_tx_buff_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER][CAN_DATA_SIZE];

uint8_t Motor::motor_tx_flag_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];
uint8_t Motor::motor_tx_map_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

Motor::Motor(const Param &param, const char *name)
    : param_(param), recv_(sizeof(can_rx_item_t), 1) {
  strncpy(this->name_, name, sizeof(this->name_));

  memset(&(this->feedback_), 0, sizeof(this->feedback_));

  switch (param.id_control) {
    case M3508_M2006_CTRL_ID_BASE:
      this->index_ = 0;
      break;
    case M3508_M2006_CTRL_ID_EXTAND:
      this->index_ = 1;
      break;
    case GM6020_CTRL_ID_EXTAND:
      this->index_ = 2;
      break;
    default:
      ASSERT(false);
  }

  om_user_fun_t rx_callback = [](om_msg_t *msg, void *arg) {
    can_rx_item_t *rx = (can_rx_item_t *)msg->buff;

    Motor *motor = (Motor *)arg;

    motor->recv_.OverwriteFromISR(rx);

    return OM_OK;
  };

  System::Message::Topic<can_rx_item_t> motor_tp(this->name_, false);

  System::Message::Subscription<can_rx_item_t> motor_sub(motor_tp, rx_callback,
                                                         this);

  bsp_can_register_subscriber(this->param_.can, motor_tp.GetHandle(),
                              this->param_.id_feedback, 0);

  motor_tx_map_[this->param_.can][this->index_] |= 1 << (this->param_.num);
}

bool Motor::Update() {
  can_rx_item_t pack;

  while (this->recv_.Receive(&pack, 0)) {
    if ((pack.index == this->param_.id_feedback) &&
        (MOTOR_NONE != this->param_.model)) {
      this->Decode(pack);
    }
  }

  return true;
}

void Motor::Decode(can_rx_item_t &rx) {
  uint16_t raw_angle = (uint16_t)((rx.data[0] << 8) | rx.data[1]);
  int16_t raw_current = (int16_t)((rx.data[4] << 8) | rx.data[5]);

  this->feedback_.rotor_abs_angle = raw_angle / (float)MOTOR_ENC_RES * M_2PI;
  this->feedback_.rotational_speed = (int16_t)((rx.data[2] << 8) | rx.data[3]);
  this->feedback_.torque_current =
      raw_current * M3508_MAX_ABS_CUR / (float)MOTOR_CUR_RES;
  this->feedback_.temp = rx.data[6];
}

float Motor::GetLSB() {
  switch (this->param_.model) {
    case MOTOR_M2006:
      return (float)M2006_MAX_ABS_LSB;

    case MOTOR_M3508:
      return (float)M3508_MAX_ABS_LSB;

    case MOTOR_GM6020:
      return (float)GM6020_MAX_ABS_LSB;

    default:
      return 0.0f;
  }
}

bool Motor::AddData() {
  float lsb = this->GetLSB();

  if (lsb != 0.0f) {
    int16_t ctrl_cmd = this->output_ * lsb;
    motor_tx_buff_[this->param_.can][this->index_][2 * this->param_.num] =
        (uint8_t)((ctrl_cmd >> 8) & 0xFF);
    motor_tx_buff_[this->param_.can][this->index_][2 * this->param_.num + 1] =
        (uint8_t)(ctrl_cmd & 0xFF);
    motor_tx_flag_[this->param_.can][this->index_] |= 1 << (this->param_.num);

    if (((~motor_tx_flag_[this->param_.can][this->index_]) &
         (motor_tx_map_[this->param_.can][this->index_])) == 0) {
      this->SendData();
    }
    return true;
  }

  return false;
}

bool Motor::SendData() {
  bsp_can_trans_packet(this->param_.can, this->param_.id_control,
                       motor_tx_buff_[this->param_.can][this->index_],
                       &this->mailbox_, 1);

  motor_tx_flag_[this->param_.can][this->index_] = 0;

  return true;
}

void Motor::Control(float out) {
  clampf(&out, -1.0f, 1.0f);
  this->output_ = out;
}

void Motor::Offline() {
  memset(&(this->feedback_), 0, sizeof(this->feedback_));
}
