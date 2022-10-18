#include "dev_mit_motor.hpp"

#include <string>

#include "comp_utils.hpp"

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

uint8_t RELAX_CMD[8] = {0X7F, 0XFF, 0X7F, 0XF0, 0X00, 0X00, 0X07, 0XFF};
uint8_t ENABLE_CMD[8] = {0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFC};

static bool initd[BSP_CAN_NUM] = {false};
Message::Topic<CAN::Pack> *MitMotor::mit_tp[BSP_CAN_NUM];

MitMotor::MitMotor(const Param &param, const char *name)
    : BaseMotor(name), param_(param), recv_(sizeof(CAN::Pack), 1) {
  auto rx_callback = [](CAN::Pack &rx, MitMotor *motor) {
    if (rx.data[0] == motor->param_.id) motor->recv_.OverwriteFromISR(&rx);

    return true;
  };

  if (!initd[this->param_.can]) {
    MitMotor::mit_tp[this->param_.can] =
        static_cast<Message::Topic<CAN::Pack> *>(
            System::Memory::Malloc(sizeof(Message::Topic<CAN::Pack>)));
    *MitMotor::mit_tp[this->param_.can] = Message::Topic<CAN::Pack>(
        (std::string("mit_motor_can") + std::to_string(this->param_.can))
            .c_str());

    CAN::Subscribe(*MitMotor::mit_tp[this->param_.can], this->param_.can, 0, 1);

    initd[this->param_.can] = true;
  }

  Message::Topic<CAN::Pack> motor_tp(name);

  motor_tp.RegisterCallback(rx_callback, this);

  motor_tp.Link(*this->mit_tp[this->param_.can]);

  CAN::Pack tx_buff;

  tx_buff.index = param.id;
  memcpy(tx_buff.data, ENABLE_CMD, sizeof(ENABLE_CMD));

  CAN::SendPack(this->param_.can, tx_buff);
}

bool MitMotor::Update() {
  CAN::Pack pack;

  while (this->recv_.Receive(&pack, 0)) {
    this->Decode(pack);
  }

  return true;
}

void MitMotor::Decode(CAN::Pack &rx) {
  if (this->param_.id != rx.data[0]) return;

  uint16_t raw_position, raw_speed, raw_current;

  raw_position = rx.data[1] << 8 | rx.data[2];

  raw_speed = (rx.data[3] << 4) | (rx.data[4] >> 4);

  raw_current = (rx.data[4] & 0x0f) << 8 | rx.data[5];

  float position, speed, current;

  position = uint_to_float(raw_position, P_MIN, P_MAX, 16);
  speed = uint_to_float(raw_speed, V_MIN, V_MAX, 12);
  current = uint_to_float(raw_current, -T_MAX, T_MAX, 12);

  this->feedback_.rotational_speed = speed;
  this->feedback_.rotor_abs_angle = position;
  this->feedback_.torque_current = current;
}

/* MIT电机协议只提供pd位置控制 */
void MitMotor::Control(float output) {
  RM_UNUSED(output);
  ASSERT(false);
}

void MitMotor::SetCurrent(float current) { this->current_ = current; }

void MitMotor::SetPos(float pos_error) {
  clampf(&pos_error, -this->param_.max_error, this->param_.max_error);

  float pos_sp = this->GetAngle() + 4 * M_PI;

  circle_add(&pos_sp, pos_error, 8 * M_PI);

  pos_sp -= 4 * M_PI;

  int p_int = float_to_uint(pos_sp, P_MIN, P_MAX, 16);
  int v_int = float_to_uint(this->param_.def_speed, V_MIN, V_MAX, 12);
  int kp_int = float_to_uint(this->param_.kp, KP_MIN, KP_MAX, 12);
  int kd_int = float_to_uint(this->param_.kd, KD_MIN, KD_MAX, 12);
  int t_int = float_to_uint(this->current_, T_MIN, T_MAX, 12);

  CAN::Pack tx_buff;

  tx_buff.index = this->param_.id;

  tx_buff.data[0] = p_int >> 8;
  tx_buff.data[1] = p_int & 0xFF;
  tx_buff.data[2] = v_int >> 4;
  tx_buff.data[3] = ((v_int & 0xF) << 4) | (kp_int >> 8);
  tx_buff.data[4] = kp_int & 0xFF;
  tx_buff.data[5] = kd_int >> 4;
  tx_buff.data[6] = ((kd_int & 0xF) << 4) | (t_int >> 8);
  tx_buff.data[7] = t_int & 0xff;

  CAN::SendPack(this->param_.can, tx_buff);
}

void MitMotor::Relax() {
  CAN::Pack tx_buff;

  tx_buff.index = this->param_.id;
  memcpy(tx_buff.data, RELAX_CMD, sizeof(ENABLE_CMD));

  CAN::SendPack(this->param_.can, tx_buff);
}
