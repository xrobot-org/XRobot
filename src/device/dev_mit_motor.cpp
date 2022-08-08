#include "dev_mit_motor.hpp"

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

MitMotor::MitMotor(const Param &param, const char *name)
    : param_(param), recv_(sizeof(can_rx_item_t), 1), Motor(name) {
  om_user_fun_t rx_callback = [](om_msg_t *msg, void *arg) {
    can_rx_item_t *rx = (can_rx_item_t *)msg->buff;

    MitMotor *motor = (MitMotor *)arg;

    motor->recv_.OverwriteFromISR(rx);

    return OM_OK;
  };

  System::Message::Topic<can_rx_item_t> motor_tp(this->name_, false);

  System::Message::Subscription<can_rx_item_t> motor_sub(motor_tp, rx_callback,
                                                         this);

  bsp_can_register_subscriber(this->param_.can, motor_tp.GetHandle(),
                              this->param_.id, 0);
}

bool MitMotor::Update() {
  can_rx_item_t pack;

  while (this->recv_.Receive(&pack, 0)) {
    this->Decode(pack);
  }

  return true;
}

void MitMotor::Decode(can_rx_item_t &rx) {
  if (this->param_.id != rx.data[0]) return;

  uint16_t raw_position, raw_speed, raw_current;

  raw_position = rx.data[1] << 8 | rx.data[2];

  raw_speed = (rx.data[3] << 4) | (rx.data[4] >> 4);

  raw_current = (rx.data[4] & 0x0f) << 8 | rx.data[5];

  float position, speed, current;

  position = uint_to_float(raw_position, P_MIN, P_MAX, 16);
  speed = uint_to_float(raw_speed, V_MIN, V_MAX, 12);
  current = uint_to_float(raw_current, -T_MAX, T_MAX, 12);

  if (this->reverse_) {
    circle_reverse(&position);
    speed = -speed;
    current = -current;
  }

  this->feedback_.rotational_speed = speed;
  this->feedback_.rotor_abs_angle = position;
  this->feedback_.torque_current = current;
}

/* MIT电机协议只提供pd位置控制 */
void MitMotor::Control(float output) { ASSERT(false); }

void MitMotor::Set(float pos_sp) {
  int p_int = float_to_uint(pos_sp, P_MIN, P_MAX, 16);
  int v_int = float_to_uint(this->param_.def_speed, V_MIN, V_MAX, 12);
  int kp_int = float_to_uint(this->param_.kp, KP_MIN, KP_MAX, 12);
  int kd_int = float_to_uint(this->param_.kd, KD_MIN, KD_MAX, 12);
  int t_int = float_to_uint(0, T_MIN, T_MAX, 12);

  uint8_t data[8];

  data[0] = p_int >> 8;
  data[1] = p_int & 0xFF;
  data[2] = v_int >> 4;
  data[3] = ((v_int & 0xF) << 4) | (kp_int >> 8);
  data[4] = kp_int & 0xFF;
  data[5] = kd_int >> 4;
  data[6] = ((kd_int & 0xF) << 4) | (t_int >> 8);
  data[7] = t_int & 0xff;

  bsp_can_trans_packet(this->param_.can, this->param_.id, data,
                       &(this->mailbox_), 1);
}
