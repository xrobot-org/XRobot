#include "mod_dual_leg.hpp"

#include "bsp_time.h"

using namespace Module;

using namespace Component::Type;

WheelLeg::WheelLeg(WheelLeg::Param &param, float sample_freq)
    : param_(param), ctrl_lock_(true) {
  constexpr auto LEG_NAMES = magic_enum::enum_names<Leg>();
  constexpr auto MOTOR_NAMES = magic_enum::enum_names<LegMotor>();
  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->leg_motor_[i * LEG_MOTOR_NUM + j] = static_cast<Device::MitMotor *>(
          System::Memory::Malloc(sizeof(Device::MitMotor)));
      new (this->leg_motor_[i * LEG_MOTOR_NUM + j])
          Device::MitMotor(this->param_.leg_motor[i * LEG_MOTOR_NUM + j],
                           ((std::string("Leg_")) + LEG_NAMES[i].data() + "_" +
                            MOTOR_NAMES[j].data())
                               .c_str());

      this->leg_actuator_[i * LEG_MOTOR_NUM + j] =
          static_cast<Component::PosActuator *>(
              System::Memory::Malloc(sizeof(Component::PosActuator)));
      new (this->leg_actuator_[i * LEG_MOTOR_NUM + j]) Component::PosActuator(
          this->param_.leg_actr[i * LEG_MOTOR_NUM + j], sample_freq);
    }
  }

  for (int i = 0; i < LEG_NUM; i++) {
    for (int j = 0; j < LEG_MOTOR_NUM; j++) {
      circle_add(this->param_.motor_zero + i * 2 + j, M_PI, M_2PI);
    }
  }

  auto event_callback = [](uint32_t event, void *arg) {
    WheelLeg *leg = static_cast<WheelLeg *>(arg);

    leg->ctrl_lock_.Take(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        leg->SetMode(RELAX);
        break;
      case SET_MODE_BREAK:
        leg->SetMode(BREAK);
        break;
      case SET_MODE_SQUAT:
        leg->SetMode(SQUAT);
        break;
      case SET_MODE_JUMP:
        leg->SetMode(JUMP);
        break;
      default:
        break;
    }

    leg->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.EVENT_MAP);

  auto leg_thread = [](WheelLeg *leg) {
    auto eulr_sub = Message::Subscriber("chassis_eulr", leg->eulr_);

    while (1) {
      eulr_sub.DumpData();

      leg->UpdateFeedback();

      leg->Control();

      leg->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(leg_thread, this, "leg_thread", 768,
                       System::Thread::MEDIUM);
}

void WheelLeg::UpdateFeedback() {
  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->leg_motor_[i * LEG_MOTOR_NUM + j]->Update();
    }
  }

  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->feedback_[i].motor_angle[j] =
          this->leg_motor_[i * LEG_MOTOR_NUM + j]->GetAngle();
      circle_add(this->feedback_[i].motor_angle + j,
                 -this->param_.motor_zero[i * LEG_MOTOR_NUM + j], M_2PI);
    }

    Polar2 polar_l2[LEG_MOTOR_NUM]{
        {this->feedback_[i].motor_angle[LEG_FRONT], this->param_.l2},
        {this->feedback_[i].motor_angle[LEG_BACK], this->param_.l2}};
    Position2 pos_l2_end[LEG_MOTOR_NUM]{
        {static_cast<Position2>(polar_l2[LEG_FRONT]) +
         Position2(-param_.l1 / 2.0f, 0.0f)},
        {static_cast<Position2>(polar_l2[LEG_BACK]) +
         Position2(param_.l1 / 2.0f, 0.0f)}};

    this->feedback_[i].diagonal =
        Line(pos_l2_end[LEG_FRONT], pos_l2_end[LEG_BACK]);

    Position2 middle_point = this->feedback_[i].diagonal.MiddlePoint();
    float length = sqrtf(powf(this->param_.l3, 2) -
                         powf(this->feedback_[i].diagonal.Length() / 2.0f, 2));
    float angle = -Component::Triangle::Supplementary(
        this->feedback_[i].diagonal.Angle());
    circle_add(&angle, M_PI / 2.0f, M_2PI);

    this->feedback_[i].whell_pos =
        Position2(Polar2(angle, length)) + middle_point;
    if (i == LEG_LEFT) {
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
    case RELAX:
    case BREAK:
      for (int i = 0; i < LEG_NUM; i++) {
        this->setpoint_[i].whell_pos.x_ = 0.0f;
        this->setpoint_[i].whell_pos.y_ = -this->param_.limit.high_min;
      }
      break;
    case SQUAT:
    case JUMP: {
      float y_err = sinf(this->eulr_.rol) * this->param_.l4 * 3.0f;

      if (this->eulr_.rol < 0.0f) {
        this->setpoint_[LEG_RIGHT].whell_pos.x_ = 0.0f;
        this->setpoint_[LEG_RIGHT].whell_pos.y_ += y_err * this->dt_;
      } else {
        this->setpoint_[LEG_LEFT].whell_pos.x_ = 0.0f;
        this->setpoint_[LEG_LEFT].whell_pos.y_ -= y_err * this->dt_;
      }

      float tmp = MAX(this->setpoint_[LEG_LEFT].whell_pos.y_,
                      this->setpoint_[LEG_RIGHT].whell_pos.y_) +
                  this->param_.limit.high_min;

      this->setpoint_[LEG_LEFT].whell_pos.y_ -= tmp;
      this->setpoint_[LEG_RIGHT].whell_pos.y_ -= tmp;

      clampf(&this->setpoint_[LEG_LEFT].whell_pos.y_,
             -this->param_.limit.high_max, -this->param_.limit.high_min);
      clampf(&this->setpoint_[LEG_RIGHT].whell_pos.y_,
             -this->param_.limit.high_max, -this->param_.limit.high_min);
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  /* 控制逻辑 */
  switch (this->mode_) {
    case RELAX:
      for (uint8_t i = 0; i < LEG_NUM; i++) {
        for (int j = 0; j < LEG_MOTOR_NUM; j++) {
          this->leg_motor_[i * LEG_MOTOR_NUM + j]->Relax();
        }
      }
      break;

    case BREAK:
    case SQUAT:
    case JUMP:
      for (uint8_t i = 0; i < LEG_NUM; i++) {
        Position2 motor_pos[LEG_MOTOR_NUM] = {{-param_.l1 / 2.0f, 0.0f},
                                              {param_.l1 / 2.0f, 0.0f}};

        for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
          Component::Type::Position2 target = this->setpoint_[i].whell_pos;

          if (i == LEG_LEFT) {
            target.x_ = -target.x_;
          }

          Component::Triangle leg_tri;

          leg_tri.data_.side[0] = this->param_.l3;
          leg_tri.data_.side[1] = this->param_.l2;
          leg_tri.data_.side[2] = Position2::Distance(target, motor_pos[j]);

          leg_tri.Slove();

          float angle = Line(motor_pos[j], target).Angle();

          if (j == LEG_FRONT) {
            circle_add(&angle, -leg_tri.data_.angle[0], M_2PI);
          } else {
            circle_add(&angle, leg_tri.data_.angle[0], M_2PI);
          }

          this->setpoint_[i].motor_angle[j] = angle;

          this->leg_motor_[i * LEG_MOTOR_NUM + j]->SetCurrent(
              this->leg_actuator_[i * LEG_MOTOR_NUM + j]->Calculate(
                  angle, this->leg_motor_[i * LEG_MOTOR_NUM + j]->GetSpeed(),
                  this->feedback_[i].motor_angle[j], this->dt_));

          this->leg_motor_[i * LEG_MOTOR_NUM + j]->SetPos(
              circle_error(angle, this->feedback_[i].motor_angle[j], M_2PI));
        }
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

void WheelLeg::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < LEG_NUM * LEG_MOTOR_NUM; i++) {
    this->leg_actuator_[i]->Reset();
  }

  this->mode_ = mode;
}
