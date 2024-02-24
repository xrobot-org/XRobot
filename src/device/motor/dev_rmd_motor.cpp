#include "dev_rmd_motor.hpp"

#include "bsp_time.h"

/* 电机最大控制输出绝对值 */
#define RMD9250_MAX_ABS_LSB (2000)

#define MOTOR_ENC_RES (16383)           /* 电机编码器分辨率 */
#define MOTOR_CUR_RES (2048.0f / 33.0f) /* 电机编码器分辨率 */

using namespace Device;

uint8_t RMDMotor::motor_tx_buff_[BSP_CAN_NUM][8];

uint8_t RMDMotor::motor_tx_flag_[BSP_CAN_NUM];

uint8_t RMDMotor::motor_tx_map_[BSP_CAN_NUM];

RMDMotor::RMDMotor(const Param &param, const char *name)
    : BaseMotor(name, param.reverse), param_(param) {
  strncpy(this->name_, name, sizeof(this->name_));

  memset(&(this->feedback_), 0, sizeof(this->feedback_));

  auto rx_callback = [](Can::Pack &rx, RMDMotor *motor) {
    motor->recv_.Overwrite(rx);

    motor->last_online_time_ = bsp_time_get_ms();

    return true;
  };

  Message::Topic<Can::Pack> motor_tp(name);

  motor_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(motor_tp, this->param_.can, this->param_.num + 0x141, 1);

  if ((motor_tx_map_[this->param_.can] & (1 << (this->param_.num))) != 0) {
    /* Error: ID duplicate */
    XB_ASSERT(false);
  }

  motor_tx_map_[this->param_.can] |= 1 << (this->param_.num);
}

bool RMDMotor::Update() {
  Can::Pack pack;

  while (this->recv_.Receive(pack)) {
    this->Decode(pack);
  }

  return true;
}

void RMDMotor::Decode(Can::Pack &rx) {
  uint16_t raw_angle = static_cast<uint16_t>((rx.data[7] << 8) | rx.data[6]);
  int16_t raw_current = static_cast<int16_t>((rx.data[3] << 8) | rx.data[2]);
  float raw_speed = static_cast<int16_t>((rx.data[5] << 8) | rx.data[4]);
  this->feedback_.rotor_abs_angle =
      static_cast<float>(raw_angle) / MOTOR_ENC_RES * M_2PI;
  this->feedback_.rotational_speed = raw_speed / 360.0f * 60.0f;
  this->feedback_.torque_current =
      static_cast<float>(raw_current) / MOTOR_CUR_RES;

  this->feedback_.temp = rx.data[1];
}

void RMDMotor::Control(float out) {
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

  float lsb = RMD9250_MAX_ABS_LSB;

  if (lsb != 0.0f) {
    int16_t ctrl_cmd = static_cast<int16_t>(this->output_ * lsb);
    motor_tx_buff_[this->param_.can][2 * this->param_.num + 1] =
        static_cast<uint8_t>((ctrl_cmd >> 8) & 0xFF);
    motor_tx_buff_[this->param_.can][2 * this->param_.num] =
        static_cast<uint8_t>(ctrl_cmd & 0xFF);
    motor_tx_flag_[this->param_.can] |= 1 << (this->param_.num);

    if (((~motor_tx_flag_[this->param_.can]) &
         (motor_tx_map_[this->param_.can])) == 0) {
      this->SendData();
    }
  }
}

bool RMDMotor::SendData() {
  Can::Pack tx_buff;

  tx_buff.index = 0x280;

  memcpy(tx_buff.data, motor_tx_buff_[this->param_.can], sizeof(tx_buff.data));

  Can::SendStdPack(this->param_.can, tx_buff);

  motor_tx_flag_[this->param_.can] = 0;

  return true;
}

void RMDMotor::Offline() {
  memset(&(this->feedback_), 0, sizeof(this->feedback_));
}

void RMDMotor::Relax() { this->Control(0.0f); }
