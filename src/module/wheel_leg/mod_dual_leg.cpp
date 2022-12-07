#include "mod_dual_leg.hpp"

#include "bsp_time.h"
#include "magic_enum.hpp"

using namespace Module;

using namespace Component::Type;

WheelLeg::WheelLeg(WheelLeg::Param& param, float sample_freq)
    : param_(param), ctrl_lock_(true) {
  constexpr auto leg_names = magic_enum::enum_names<Leg>();
  constexpr auto motor_names = magic_enum::enum_names<LegMotor>();
  for (uint8_t i = 0; i < LegNum; i++) {
    for (uint8_t j = 0; j < LegMotorNum; j++) {
      this->leg_motor_[i * LegMotorNum + j] =
          (Device::MitMotor*)System::Memory::Malloc(sizeof(Device::MitMotor));
      new (this->leg_motor_[i * LegMotorNum + j])
          Device::MitMotor(this->param_.leg_motor[i * LegMotorNum + j],
                           (((std::string) "Leg_") + leg_names[i].data() + "_" +
                            motor_names[j].data())
                               .c_str());

      this->leg_actuator_[i * LegMotorNum + j] =
          (Component::PosActuator*)System::Memory::Malloc(
              sizeof(Component::PosActuator));
      new (this->leg_actuator_[i * LegMotorNum + j]) Component::PosActuator(
          this->param_.leg_actr[i * LegMotorNum + j], sample_freq);
    }
  }

  for (int i = 0; i < LegNum; i++) {
    for (int j = 0; j < LegMotorNum; j++) {
      circle_add(this->param_.motor_zero + i * 2 + j, M_PI, M_2PI);
    }
  }

  auto event_callback = [](uint32_t event, void* arg) {
    WheelLeg* leg = static_cast<WheelLeg*>(arg);

    leg->ctrl_lock_.Take(UINT32_MAX);

    switch (event) {
      case ChangeModeRelax:
        leg->SetMode(Relax);
        break;
      case ChangeModeBreak:
        leg->SetMode(Break);
        break;
      case ChangeModeSquat:
        leg->SetMode(Squat);
        break;
      case ChangeModeJump:
        leg->SetMode(Jump);
        break;
      default:
        break;
    }

    leg->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.event_map);

  auto leg_thread = [](WheelLeg* leg) {
    auto eulr_sub = Message::Subscriber("chassis_eulr", leg->eulr_);

    while (1) {
      eulr_sub.DumpData();

      leg->UpdateFeedback();

      leg->Control();

      leg->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(leg_thread, this, "leg_thread", 768,
                       System::Thread::Medium);
}

void WheelLeg::UpdateFeedback() {
  for (uint8_t i = 0; i < LegNum; i++)
    for (uint8_t j = 0; j < LegMotorNum; j++)
      this->leg_motor_[i * LegMotorNum + j]->Update();

  for (uint8_t i = 0; i < LegNum; i++) {
    for (uint8_t j = 0; j < LegMotorNum; j++) {
      this->feedback_[i].motor_angle[j] =
          this->leg_motor_[i * LegMotorNum + j]->GetAngle();
      circle_add(this->feedback_[i].motor_angle + j,
                 -this->param_.motor_zero[i * LegMotorNum + j], M_2PI);
    }

    Polar2 polar_l2[LegMotorNum]{
        {this->feedback_[i].motor_angle[Front], this->param_.l2},
        {this->feedback_[i].motor_angle[Back], this->param_.l2}};
    Position2 pos_l2_end[LegMotorNum]{
        {Position2(polar_l2[Front]) + Position2(-param_.l1 / 2.0f, 0.0f)},
        {Position2(polar_l2[Back]) + Position2(param_.l1 / 2.0f, 0.0f)}};

    this->feedback_[i].diagonal = Line(pos_l2_end[Front], pos_l2_end[Back]);

    Position2 middle_point = this->feedback_[i].diagonal.MiddlePoint();
    float _length = sqrtf(powf(this->param_.l3, 2) -
                          powf(this->feedback_[i].diagonal.Length() / 2.0f, 2));
    float _angle = -Component::Triangle::Supplementary(
        this->feedback_[i].diagonal.Angle());
    circle_add(&_angle, M_PI / 2.0f, M_2PI);

    this->feedback_[i].whell_pos =
        (Position2)Polar2(_angle, _length) + middle_point;
    if (i == Left) {
      this->feedback_[i].whell_pos.x_ = -this->feedback_[i].whell_pos.x_;
    }
    this->feedback_[i].whell_polar = Polar2(this->feedback_[i].whell_pos);
  }
}

void WheelLeg::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* 处理控制命令 */
  switch (this->mode_) {
    case Relax:
    case Break:
      for (int i = 0; i < LegNum; i++) {
        this->setpoint_[i].whell_pos.x_ = 0.0f;
        this->setpoint_[i].whell_pos.y_ = -this->param_.limit.high_min;
      }
      break;
    case Squat:
    case Jump: {
      float y_err = sinf(this->eulr_.rol) * this->param_.l4 * 3.0f;

      if (this->eulr_.rol < 0.0f) {
        this->setpoint_[Right].whell_pos.x_ = 0.0f;
        this->setpoint_[Right].whell_pos.y_ += y_err * this->dt_;
      } else {
        this->setpoint_[Left].whell_pos.x_ = 0.0f;
        this->setpoint_[Left].whell_pos.y_ -= y_err * this->dt_;
      }

      float tmp = MAX(this->setpoint_[Left].whell_pos.y_,
                      this->setpoint_[Right].whell_pos.y_) +
                  this->param_.limit.high_min;

      this->setpoint_[Left].whell_pos.y_ -= tmp;
      this->setpoint_[Right].whell_pos.y_ -= tmp;

      clampf(&this->setpoint_[Left].whell_pos.y_, -this->param_.limit.high_max,
             -this->param_.limit.high_min);
      clampf(&this->setpoint_[Right].whell_pos.y_, -this->param_.limit.high_max,
             -this->param_.limit.high_min);
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  /* 控制逻辑 */
  switch (this->mode_) {
    case Relax:
      for (uint8_t i = 0; i < LegNum; i++) {
        for (int j = 0; j < LegMotorNum; j++) {
          this->leg_motor_[i * LegMotorNum + j]->Relax();
        }
      }
      break;

    case Break:
    case Squat:
    case Jump:
      for (uint8_t i = 0; i < LegNum; i++) {
        Position2 motor_pos[LegMotorNum] = {{-param_.l1 / 2.0f, 0.0f},
                                            {param_.l1 / 2.0f, 0.0f}};

        for (uint8_t j = 0; j < LegMotorNum; j++) {
          Component::Type::Position2 target = this->setpoint_[i].whell_pos;

          if (i == Left) {
            target.x_ = -target.x_;
          }

          Component::Triangle leg_tri;

          leg_tri.data_.side[0] = this->param_.l3;
          leg_tri.data_.side[1] = this->param_.l2;
          leg_tri.data_.side[2] = Position2::Distance(target, motor_pos[j]);

          leg_tri.Slove();

          float _angle = Line(motor_pos[j], target).Angle();

          if (j == Front) {
            circle_add(&_angle, -leg_tri.data_.angle[0], M_2PI);
          } else {
            circle_add(&_angle, leg_tri.data_.angle[0], M_2PI);
          }

          this->setpoint_[i].motor_angle[j] = _angle;

          this->leg_motor_[i * LegMotorNum + j]->SetCurrent(
              this->leg_actuator_[i * LegMotorNum + j]->Calculate(
                  _angle, this->leg_motor_[i * LegMotorNum + j]->GetSpeed(),
                  this->feedback_[i].motor_angle[j], this->dt_));

          this->leg_motor_[i * LegMotorNum + j]->SetPos(
              circle_error(_angle, this->feedback_[i].motor_angle[j], M_2PI));
        }
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

void WheelLeg::SetMode(Mode mode) {
  if (mode == this->mode_) return; /* 模式未改变直接返回 */

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < LegNum * LegMotorNum; i++) {
    this->leg_actuator_[i]->Reset();
  }

  this->mode_ = mode;
}
