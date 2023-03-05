#include <comp_filter.hpp>
#include <comp_pid.hpp>
#include <comp_type.hpp>

#include "comp_actuator.hpp"
#include "dev_motor.hpp"
#include "dev_rm_motor.hpp"
#include "device.hpp"

namespace Device {
template <typename Motor, typename MotorParam, int Num>
class AutoCaliLimitedMech {
 public:
  typedef struct {
    Component::ActuatorStallDetect::Param stall_detect;
    std::array<Component::PosActuator::Param, Num> pos_actuator;
    std::array<MotorParam, Num> motor_param;
    std::array<const char*, Num> motor_name;
    float cali_speed;
    float max_range;
    float margin_error;
    float reduction_ratio;
  } Param;

  AutoCaliLimitedMech(Param& param, float control_freq, float cutoff_freq)
      : param_(param) {
    for (int i = 0; i < Num; i++) {
      this->motor_[i] = new Motor(param.motor_param[i], param.motor_name[i]);
      this->pos_actuator_[i] =
          new Component::PosActuator(param.pos_actuator[i], control_freq);
      this->stall_detect_[i] =
          new Component::ActuatorStallDetect(param.stall_detect);
      this->speed_filter_[i] = new Component::LowPassFilter(cutoff_freq);
      this->current_filter_[i] = new Component::LowPassFilter(cutoff_freq);
    }
  }

  void UpdateFeedback() {
    for (int i = 0; i < Num; i++) {
      this->motor_[i]->Update();
      this->position_[i] +=
          (this->motor_[i]->GetAngle() - this->last_motor_pos_[i]) /
          param_.reduction_ratio;
      this->last_motor_pos_[i] = this->motor_[i]->GetAngle();
    }
  }

  void Control(float position, float dt) {
    if (!need_cali_) {
      clampf(&position, param_.margin_error,
             param_.max_range - param_.margin_error);
      for (int i = 0; i < Num; i++) {
        this->out_[i] = pos_actuator_[i]->Calculate(
            position, motor_[i]->GetSpeed(), position_[i], dt);
        motor_[i]->Control(out_[i]);
      }
    } else {
      bool stall = true;

      for (int i = 0; i < Num; i++) {
        bool motor_stall = this->stall_detect_[i]->Calculate(
            speed_filter_[i]->Apply(motor_[i]->GetSpeed(), dt),
            current_filter_[i]->Apply(motor_[i]->GetCurrent(), dt),
            motor_[i]->GetTemp(), dt);
        stall *= motor_stall;

        if (!motor_stall) {
          this->out_[i] = pos_actuator_[i]->SpeedCalculate(
              this->param_.cali_speed, motor_[i]->GetSpeed(), dt);
          motor_[i]->Control(out_[i]);
        }
      }

      if (stall) {
        this->need_cali_ = false;

        for (int i = 0; i < Num; i++) {
          this->position_[i] = 0.0f;
          this->motor_[i]->Relax();
        }
      }
    }
  }

  void Relax() {
    for (auto motor : motor_) {
      motor->Relax();
    }
  }

  bool Ready() { return !need_cali_; }

 private:
  bool need_cali_ = true;

  std::array<float, Num> position_;

  std::array<float, Num> out_;

  std::array<Component::Type::CycleValue, Num> last_motor_pos_;

  std::array<BaseMotor*, Num> motor_;

  Param& param_;

  std::array<Component::ActuatorStallDetect*, Num> stall_detect_;
  std::array<Component::PosActuator*, Num> pos_actuator_;

  std::array<Component::LowPassFilter*, Num> speed_filter_;
  std::array<Component::LowPassFilter*, Num> current_filter_;
};

}  // namespace Device
