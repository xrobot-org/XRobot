#include "dev_mit_motor.hpp"

#include "bsp_time.h"

#define P_MIN -12.5f
#define P_MAX 12.5f
#define V_MIN -45.0f
#define V_MAX 45.0f
#define KP_MIN 0.0f
#define KP_MAX 500.0f
#define KD_MIN 0.0f
#define KD_MAX 5.0f
#define T_MIN -18.0f
#define T_MAX 18.0f

using namespace Device;

static const uint8_t RELAX_CMD[8] = {0X7F, 0XFF, 0X7F, 0XF0,
                                     0X00, 0X00, 0X07, 0XFF};

static const uint8_t ENABLE_CMD[8] = {0XFF, 0XFF, 0XFF, 0XFF,
                                      0XFF, 0XFF, 0XFF, 0XFC};
/*

static const uint8_t RESET_CMD[8] = {0XFF, 0XFF, 0XFF, 0XFF,
                                     0XFF, 0XFF, 0XFF, 0XFD};
*/

static std::array<bool, BSP_CAN_NUM> initd = {false};

std::array<Message::Topic<Can::Pack> *, BSP_CAN_NUM> MitMotor::mit_tp_;

MitMotor::MitMotor(const Param &param, const char *name)
    : BaseMotor(name, param.reverse), param_(param) {
  auto rx_callback = [](Can::Pack &rx, MitMotor *motor) {
    if (rx.data[0] == motor->param_.id) {
      motor->recv_.Overwrite(rx);
    }

    return true;
  };

  if (!initd[this->param_.can]) {
    MitMotor::mit_tp_[this->param_.can] =
        static_cast<Message::Topic<Can::Pack> *>(
            System::Memory::Malloc(sizeof(Message::Topic<Can::Pack>)));
    *MitMotor::mit_tp_[this->param_.can] = Message::Topic<Can::Pack>(
        (std::string("mit_motor_can") + std::to_string(this->param_.can))
            .c_str());

    Can::Subscribe(*MitMotor::mit_tp_[this->param_.can], this->param_.can, 0,
                   1);

    initd[this->param_.can] = true;
  }

  Message::Topic<Can::Pack> motor_tp(name);

  motor_tp.RegisterCallback(rx_callback, this);

  motor_tp.Link(*this->mit_tp_[this->param_.can]);
}

bool MitMotor::Update() {
  Can::Pack pack;

  while (this->recv_.Receive(pack)) {
    this->Decode(pack);
    last_online_time_ = bsp_time_get_ms();
  }

  return true;
}

void MitMotor::Decode(Can::Pack &rx) {
  if (this->param_.id != rx.data[0]) {
    return;
  }

  uint16_t raw_position = rx.data[1] << 8 | rx.data[2];

  uint16_t raw_speed = (rx.data[3] << 4) | (rx.data[4] >> 4);

  uint16_t raw_current = (rx.data[4] & 0x0f) << 8 | rx.data[5];

  raw_pos_ = uint_to_float(raw_position, P_MIN, P_MAX, 16);
  float speed = uint_to_float(raw_speed, V_MIN, V_MAX, 12);
  float current = uint_to_float(raw_current, -T_MAX, T_MAX, 12);

  this->feedback_.rotational_speed = speed;
  this->feedback_.rotor_abs_angle = raw_pos_;
  this->feedback_.torque_current = current;
}

/* MIT电机协议只提供pd位置控制 */
void MitMotor::Control(float output) {
  static_cast<void>(output);
  XB_ASSERT(false);
}

void MitMotor::SetCurrent(float current) {
  if (this->feedback_.temp > 75.0f) {
    Relax();
    OMLOG_WARNING("motor %s high temperature detected", name_);
    return;
  }

  if (reverse_) {
    this->current_ = -current;
  } else {
    this->current_ = current;
  }
}

void MitMotor::SetPos(float pos) {
  if (this->feedback_.temp > 75.0f) {
    Relax();
    OMLOG_WARNING("motor %s high temperature detected", name_);
    return;
  }

  float pos_sp = Component::Type::CycleValue(pos) - this->GetAngle();

  if (reverse_) {
    pos_sp = -pos_sp;
  }

  clampf(&pos_sp, -param_.max_error, param_.max_error);

  pos_sp += this->raw_pos_;

  while (pos_sp > 4 * M_PI) {
    pos_sp -= 8 * M_PI;
  }

  while (pos_sp < -4 * M_PI) {
    pos_sp += 8 * M_PI;
  }

  int p_int = float_to_uint(pos_sp, P_MIN, P_MAX, 16);
  int v_int = float_to_uint(this->param_.def_speed, V_MIN, V_MAX, 12);
  int kp_int = float_to_uint(this->param_.kp, KP_MIN, KP_MAX, 12);
  int kd_int = float_to_uint(this->param_.kd, KD_MIN, KD_MAX, 12);
  int t_int = float_to_uint(this->current_, T_MIN, T_MAX, 12);

  Can::Pack tx_buff;

  tx_buff.index = this->param_.id;

  tx_buff.data[0] = p_int >> 8;
  tx_buff.data[1] = p_int & 0xFF;
  tx_buff.data[2] = v_int >> 4;
  tx_buff.data[3] = ((v_int & 0xF) << 4) | (kp_int >> 8);
  tx_buff.data[4] = kp_int & 0xFF;
  tx_buff.data[5] = kd_int >> 4;
  tx_buff.data[6] = ((kd_int & 0xF) << 4) | (t_int >> 8);
  tx_buff.data[7] = t_int & 0xff;

  Can::SendStdPack(this->param_.can, tx_buff);
}

void MitMotor::Relax() {
  Can::Pack tx_buff;

  tx_buff.index = this->param_.id;

  memcpy(tx_buff.data, RELAX_CMD, sizeof(RELAX_CMD));

  Can::SendStdPack(this->param_.can, tx_buff);
}

void MitMotor::Enable() {
  Can::Pack tx_buff;

  tx_buff.index = param_.id;

  memcpy(tx_buff.data, ENABLE_CMD, sizeof(ENABLE_CMD));

  Can::SendStdPack(this->param_.can, tx_buff);
}
