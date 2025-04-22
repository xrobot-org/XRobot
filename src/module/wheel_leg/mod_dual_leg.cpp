#include "mod_dual_leg.hpp"

#include <comp_type.hpp>
#include <comp_utils.hpp>

#include "bsp_time.h"

using namespace Module;

using namespace Component::Type;

WheelLeg::WheelLeg(WheelLeg::Param &param, float sample_freq)
    : param_(param), wheel_polor_("leg_whell_polor"), ctrl_lock_(true) {
  constexpr auto LEG_NAMES = magic_enum::enum_names<Leg>();
  constexpr auto MOTOR_NAMES = magic_enum::enum_names<LegMotor>();
  for (uint8_t i = 0; i < LEG_NUM; i++) {
    for (uint8_t j = 0; j < LEG_MOTOR_NUM; j++) {
      this->leg_motor_[i * LEG_MOTOR_NUM + j] =
          new Device::MitMotor(this->param_.leg_motor.at(i * LEG_MOTOR_NUM + j),
                               ((std::string("Leg_")) + LEG_NAMES[i].data() +
                                "_" + MOTOR_NAMES[j].data())
                                   .c_str());

      this->leg_actuator_.at(i * LEG_MOTOR_NUM + j) =
          new Component::PosActuator(
              this->param_.leg_actr.at(i * LEG_MOTOR_NUM + j), sample_freq);
    }
  }

  for (int i = 0; i < LEG_NUM; i++) {
    for (int j = 0; j < LEG_MOTOR_NUM; j++) {
      this->param_.motor_zero.at(i * 2 + j) += M_PI;
    }
  }

  auto event_callback = [](ChassisEvent event, WheelLeg *leg) {
    leg->ctrl_lock_.Wait(UINT32_MAX);

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

    leg->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<WheelLeg *, ChassisEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto leg_thread = [](WheelLeg *leg) {
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("chassis_eulr");

    auto gyro_sub =
        Message::Subscriber<Component::Type::Vector3>("chassis_gyro");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      eulr_sub.DumpData(leg->eulr_);
      gyro_sub.DumpData(leg->gyro_);

      leg->UpdateFeedback();

      leg->Control();

      leg->wheel_polor_.Publish(leg->feedback_[0].whell_polar);

      leg->thread_.SleepUntil(5, last_online_time);
    }
  };

  this->thread_.Create(leg_thread, this, "leg_thread",
                       MODULE_WHEELLEG_TASK_STACK_DEPTH,
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
      this->feedback_[i].motor_angle[j] -=
          this->param_.motor_zero[i * LEG_MOTOR_NUM + j];
    }

    std::array<Polar2, LEG_MOTOR_NUM> polar_l2{
        Polar2{this->feedback_[i].motor_angle[LEG_FRONT], this->param_.l2},
        Polar2{this->feedback_[i].motor_angle[LEG_BACK], this->param_.l2}};
    std::array<Position2, LEG_MOTOR_NUM> pos_l2_end{
        Position2{static_cast<Position2>(polar_l2[LEG_FRONT]) +
                  Position2(-param_.l1 / 2.0f, 0.0f)},
        Position2{static_cast<Position2>(polar_l2[LEG_BACK]) +
                  Position2(param_.l1 / 2.0f, 0.0f)}};

    this->feedback_[i].diagonal =
        Line(pos_l2_end[LEG_FRONT], pos_l2_end[LEG_BACK]);

    Position2 middle_point = this->feedback_[i].diagonal.MiddlePoint();

    float length = sqrtf(powf(this->param_.l3, 2) -
                         powf(this->feedback_[i].diagonal.Length() / 2.0f, 2));
    Component::Type::CycleValue angle = -Component::Triangle::Supplementary(
        this->feedback_[i].diagonal.Angle());
    angle += M_PI / 2.0f;

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

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  /* 处理控制命令 */
  switch (this->mode_) {
    case RELAX:
    case BREAK: {
      for (int i = 0; i < LEG_NUM; i++) {
        this->setpoint_[i].whell_pos.x_ = 0.0f;
        this->setpoint_[i].whell_pos.y_ = -this->param_.limit.high_min;
      }
    }

    break;
    case SQUAT:
    case JUMP: {
      float angle = eulr_.pit - Component::Type::CycleValue(0.15f);

      if (fabsf(angle) < 0.05f) {
        angle = 0;
      } else {
        if (angle > 0) {
          angle -= 0.05f;
        } else {
          angle += 0.05f;
        }
      }

      Component::Type::Polar2 target_wheel_polor(
          angle - static_cast<float>(M_PI) * 0.5f, param_.limit.high_min);
      clampf(&target_wheel_polor.angle_, -0.2f - M_PI * 0.5, 0.2f - M_PI * 0.5);
      setpoint_[LEG_RIGHT].whell_pos = setpoint_[LEG_LEFT].whell_pos =
          target_wheel_polor;
      break;
    }
    default:
      XB_ASSERT(false);
      return;
  }

  /* 控制逻辑 */
  switch (this->mode_) {
    case RELAX:
      for (uint8_t i = 0; i < LEG_NUM; i++) {
        for (int j = 0; j < LEG_MOTOR_NUM; j++) {
          this->leg_motor_[i * LEG_MOTOR_NUM + j]->Relax();
          System::Thread::Sleep(1);
        }
      }
      break;

    case BREAK:
    case SQUAT:
    case JUMP:
      for (uint8_t i = 0; i < LEG_NUM; i++) {
        std::array<Position2, LEG_MOTOR_NUM> motor_pos = {
            Position2{-param_.l1 / 2.0f, 0.0f},
            Position2{param_.l1 / 2.0f, 0.0f}};

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

          Component::Type::CycleValue angle =
              Line(motor_pos[j], target).Angle();

          if (j == LEG_FRONT) {
            angle -= leg_tri.data_.angle[0];
          } else {
            angle += leg_tri.data_.angle[0];
          }

          this->setpoint_[i].motor_angle[j] = angle;

          this->leg_motor_[i * LEG_MOTOR_NUM + j]->SetCurrent(
              this->leg_actuator_[i * LEG_MOTOR_NUM + j]->Calculate(
                  angle, this->leg_motor_[i * LEG_MOTOR_NUM + j]->GetSpeed(),
                  this->feedback_[i].motor_angle[j], this->dt_));

          this->leg_motor_[i * LEG_MOTOR_NUM + j]->SetPos(
              angle + this->param_.motor_zero[i * LEG_MOTOR_NUM + j]);
          System::Thread::Sleep(1);
        }
      }
      break;
    default:
      XB_ASSERT(false);
      return;
  }
}

void WheelLeg::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  for (auto motor : this->leg_motor_) {
    motor->Enable();
  }

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < LEG_NUM * LEG_MOTOR_NUM; i++) {
    this->leg_actuator_[i]->Reset();
  }

  this->mode_ = mode;
}
