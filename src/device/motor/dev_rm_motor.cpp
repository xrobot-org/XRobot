#include "dev_rm_motor.hpp"

#include "bsp_time.h"

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

uint8_t RMMotor::motor_tx_buff_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER][8];

uint8_t RMMotor::motor_tx_flag_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

uint8_t RMMotor::motor_tx_map_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

RMMotor::RMMotor(const Param &param, const char *name)
    : BaseMotor(name, param.reverse), param_(param) {
  strncpy(this->name_, name, sizeof(this->name_));

  memset(&(this->feedback_), 0, sizeof(this->feedback_));

  switch (param.id_control) {
    case M3508_M2006_CTRL_ID_BASE:
      this->index_ = 0;
      XB_ASSERT(param.id_feedback > 0x200 && param.id_feedback <= 0x204);
      break;
    case M3508_M2006_CTRL_ID_EXTAND:
      this->index_ = 1;
      XB_ASSERT(param.id_feedback > 0x204 && param.id_feedback <= 0x208);
      break;
    case GM6020_CTRL_ID_EXTAND:
      this->index_ = 2;
      XB_ASSERT(param.id_feedback > 0x208 && param.id_feedback <= 0x20B);
      break;
    default:
      XB_ASSERT(false);
  }
  switch (param.model) {
    case MOTOR_M2006:
    case MOTOR_M3508:
      if (param.id_control == M3508_M2006_CTRL_ID_BASE) {
        this->num_ = param.id_feedback - M3508_M2006_FB_ID_BASE;
      } else {
        this->num_ = param.id_feedback - M3508_M2006_FB_ID_EXTAND;
      }
      break;
    case MOTOR_GM6020:
      if (param.id_control == GM6020_CTRL_ID_BASE) {
        this->num_ = param.id_feedback - GM6020_FB_ID_BASE;
      } else {
        this->num_ = param.id_feedback - GM6020_FB_ID_EXTAND;
      }
      break;
    default:
      break;
  }

  auto rx_callback = [](Can::Pack &rx, RMMotor *motor) {
    motor->recv_.Overwrite(rx);

    motor->last_online_time_ = bsp_time_get_ms();

    return true;
  };

  Message::Topic<Can::Pack> motor_tp(name);

  motor_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(motor_tp, this->param_.can, this->param_.id_feedback, 1);

  if ((motor_tx_map_[this->param_.can][this->index_] & (1 << (this->num_))) !=
      0) {
    /* Error: ID duplicate */
    XB_ASSERT(false);
  }

  motor_tx_map_[this->param_.can][this->index_] |= 1 << (this->num_);
}

bool RMMotor::Update() {
  Can::Pack pack;

  while (this->recv_.Receive(pack)) {
    if ((pack.index == this->param_.id_feedback) &&
        (MOTOR_NONE != this->param_.model)) {
      this->Decode(pack);
    }
  }

  return true;
}

void RMMotor::Decode(Can::Pack &rx) {
  uint16_t raw_angle = static_cast<uint16_t>((rx.data[0] << 8) | rx.data[1]);
  int16_t raw_current = static_cast<int16_t>((rx.data[4] << 8) | rx.data[5]);

  this->feedback_.rotor_abs_angle =
      static_cast<float>(raw_angle) / MOTOR_ENC_RES * M_2PI;
  this->feedback_.rotational_speed =
      static_cast<int16_t>((rx.data[2] << 8) | rx.data[3]);
  this->feedback_.torque_current =
      static_cast<float>(raw_current) * M3508_MAX_ABS_CUR / MOTOR_CUR_RES;
  this->feedback_.temp = rx.data[6];
}

float RMMotor::GetLSB() {
  switch (this->param_.model) {
    case MOTOR_M2006:
      return M2006_MAX_ABS_LSB;

    case MOTOR_M3508:
      return M3508_MAX_ABS_LSB;

    case MOTOR_GM6020:
      return GM6020_MAX_ABS_LSB;

    default:
      return 0.0f;
  }
}

void RMMotor::Control(float out) {
  if (this->feedback_.temp > 75.0f) {
    out = 0.0f;
    OMLOG_WARNING("motor %s high temperature detected", name_);
  }

  clampf(&out, -1.0f, 1.0f);
  if (reverse_) {
    this->output_ = -out;
  } else {
    this->output_ = out;
  }
  float lsb = this->GetLSB();

  if (lsb != 0.0f) {
    int16_t ctrl_cmd = static_cast<int16_t>(this->output_ * lsb);
    motor_tx_buff_[this->param_.can][this->index_][2 * this->num_] =
        static_cast<uint8_t>((ctrl_cmd >> 8) & 0xFF);
    motor_tx_buff_[this->param_.can][this->index_][2 * this->num_ + 1] =
        static_cast<uint8_t>(ctrl_cmd & 0xFF);
    motor_tx_flag_[this->param_.can][this->index_] |= 1 << (this->num_);

    if (((~motor_tx_flag_[this->param_.can][this->index_]) &
         (motor_tx_map_[this->param_.can][this->index_])) == 0) {
      this->SendData();
    }
  }
}

bool RMMotor::SendData() {
  Can::Pack tx_buff;

  tx_buff.index = this->param_.id_control;

  memcpy(tx_buff.data, motor_tx_buff_[this->param_.can][this->index_],
         sizeof(tx_buff.data));

  Can::SendStdPack(this->param_.can, tx_buff);

  motor_tx_flag_[this->param_.can][this->index_] = 0;

  return true;
}

void RMMotor::Offline() {
  memset(&(this->feedback_), 0, sizeof(this->feedback_));
}

void RMMotor::Relax() { this->Control(0.0f); }
