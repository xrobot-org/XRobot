#include "mod_dual_leg.hpp"

using namespace Module;

using namespace Component::Type;

void BalanceChassis::UpdateFeedback() {
  for (uint8_t i = 0; i < LEG_NUM; i++)
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++)
      this->leg_motor_[i * LEG_MOTOR_NUM + j]->Update();

  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->feedback_[i].motor_angle[j] =
          this->leg_motor_[i * LEG_MOTOR_NUM + j]->GetAngle();
      circle_add(this->feedback_[i].motor_angle + j,
                 -this->param_.mech_zero[i * LEG_MOTOR_NUM + j], M_2PI);
    }

    Polar2 polar_l2[LEG_MOTOR_NUM]{
        {this->feedback_[i].motor_angle[FRONT], this->param_.l2},
        {this->feedback_[i].motor_angle[BACK], this->param_.l2}};
    Position2 pos_l2_end[LEG_MOTOR_NUM]{
        {Position2(polar_l2[FRONT]) + Position2(-param_.l1 / 2.0f, 0.0f)},
        {Position2(polar_l2[BACK]) + Position2(param_.l1 / 2.0f, 0.0f)}};

    this->feedback_[i].diagonal = Line(pos_l2_end[FRONT], pos_l2_end[BACK]);

    Position2 middle_point = this->feedback_[i].diagonal.MiddlePoint();
    float _length = sqrtf(powf(this->param_.l3, 2) -
                          powf(this->feedback_[i].diagonal.Length() / 2.0f, 2));
    float _angle = -Component::Triangle::Supplementary(
        this->feedback_[i].diagonal.Angle());
    circle_add(&_angle, M_PI / 2.0f, M_2PI);

    this->feedback_[i].whell_pos =
        (Position2)Polar2(_angle, _length) + middle_point;
    this->feedback_[i].whell_polar = Polar2(this->feedback_[i].whell_pos);
  }
}

BalanceChassis::BalanceChassis(BalanceChassis::Param& param, float sample_freq)
    : param_(param) {
  this->setpoint_[0].whell_pos.x_ = 0;
  this->setpoint_[0].motor_angle[1] = 1.8;

  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->leg_motor_[i * LEG_MOTOR_NUM + j] =
          (Device::MitMotor*)System::Memory::Malloc(sizeof(Device::MitMotor));
      new (this->leg_motor_[i * LEG_MOTOR_NUM + j]) Device::MitMotor(
          this->param_.leg_motor[i * LEG_MOTOR_NUM + j],
          ("chassis_" + std::to_string(i) + "_" + std::to_string(j)).c_str());
    }
  }

  auto chassis_thread = [](void* arg) {
    BalanceChassis* chassis = (BalanceChassis*)arg;

    while (1) {
      chassis->UpdateFeedback();
      chassis->Control();

      chassis->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, chassis_thread, 768, System::Thread::Medium,
                 this);
}

void BalanceChassis::Control() {
  for (uint8_t i = 0; i < LEG_NUM; i++) {
    Position2 motor_pos[LEG_MOTOR_NUM] = {{-param_.l1 / 2.0f, 0.0f},
                                          {param_.l1 / 2.0f, 0.0f}};

    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      Component::Triangle leg_tri;

      leg_tri.data_.side[0] = this->param_.l3;
      leg_tri.data_.side[1] = this->param_.l2;
      leg_tri.data_.side[2] =
          Position2::Distance(this->setpoint_[i].whell_pos, motor_pos[j]);

      leg_tri.Slove();

      float _angle = Line(motor_pos[j], this->setpoint_[i].whell_pos).Angle();

      if (j == FRONT) {
        circle_add(&_angle, -leg_tri.data_.angle[0], M_2PI);
      } else {
        circle_add(&_angle, leg_tri.data_.angle[0], M_2PI);
      }

      this->setpoint_[i].motor_angle[j] = _angle;

      this->leg_motor_[i * LEG_MOTOR_NUM + j]->Set(
          circle_error(_angle, this->feedback_[i].motor_angle[j], M_2PI));
    }
  }
}
