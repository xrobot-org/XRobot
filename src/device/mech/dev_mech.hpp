#pragma once

#include <comp_filter.hpp>
#include <comp_pid.hpp>
#include <comp_type.hpp>
#include <comp_utils.hpp>

#include "bsp_time.h"
#include "comp_actuator.hpp"
#include "comp_trans.hpp"
#include "dev_microswitch.hpp"
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

class LimitCheck {
 public:
  LimitCheck() {}
  virtual bool ReachLimit() = 0;
};

/* 微动开关限位，到达限位点时开关闭合 */
class MicroSwitchLimit : public LimitCheck {
 public:
  typedef struct {
    uint8_t id;
  } Param;

  MicroSwitchLimit(Param param) : LimitCheck() {
    sw_data_.status = MicroSwitch::OFF;
    MicroSwitch::Subscriber(sw_data_, param.id);
  }

  bool ReachLimit() { return sw_data_.status == MicroSwitch::ON; }

  MicroSwitch::Data sw_data_{};
};

/* 自归位限位，在重力作用下自由运动一段时间后就为零点 */
class AutoReturnLimit : public LimitCheck {
 public:
  typedef struct {
    float timeout;
  } Param;

  AutoReturnLimit(Param param)
      : LimitCheck(), param_(param), start_time_(bsp_time_get()) {}

  bool ReachLimit() {
    return TIME_DIFF(start_time_, bsp_time_get()) > param_.timeout;
  }

  Param param_;

  uint64_t start_time_;
};

typedef enum { AXIS_X, AXIS_Y, AXIS_Z, AXIS_NUM } Axis;

typedef struct {
  Component::Trans::Angle target_angle_;
  Component::Type::Vector3 target_pos_;
  Component::Trans::Angle angle_;
  Component::Type::Vector3 pos_;
} PosStream;

template <typename MotorType, typename LimitType, int Num>
class SteeringMech {
 public:
  typedef struct {
    std::array<typename LimitType::Param, Num> limit_param;
    std::array<Component::PosActuator::Param, Num> pos_actuator;
    std::array<typename MotorType::Param, Num> motor_param;
    std::array<const char*, Num> motor_name;
    float cali_speed;      /* 校准时的电机运动速度 */
    float min_angle;       /* 角度最小值 */
    float max_angle;       /* 角度最大值 */
    float margin_error;    /* 正常运动时距离边界的最小距离 */
    float zero_angle;      /* 触发限位时的角度 */
    float reduction_ratio; /* 总减速比 */
    /* 角度为0时下级机构转动轴距离当前机构转动轴的坐标 */
    Component::Type::Vector3 translation;
    Axis axis; /* 转动轴 */
  } Param;

  SteeringMech(Param& param, float control_freq) : param_(param) {
    for (int i = 0; i < Num; i++) {
      this->motor_[i] =
          new MotorType(param.motor_param[i], param_.motor_name[i]);
      this->pos_actuator_[i] =
          new Component::PosActuator(param.pos_actuator[i], control_freq);
      this->limit_check_[i] = new LimitType(param.limit_param[i]);
    }
  }

  void UpdateFeedback() {
    for (int i = 0; i < Num; i++) {
      this->motor_[i]->Update();
      this->angle_[i] +=
          (this->motor_[i]->GetAngle() - this->last_motor_pos_[i]) /
          param_.reduction_ratio;
      this->last_motor_pos_[i] = this->motor_[i]->GetAngle();
    }
  }

  void Control(float angle, float dt) {
    if (!need_cali_) {
      if (angle > param_.max_angle - param_.margin_error) {
        angle = param_.max_angle - param_.margin_error;
      } else if (angle < param_.min_angle + param_.margin_error) {
        angle = param_.min_angle + param_.margin_error;
      }

      for (int i = 0; i < Num; i++) {
        this->out_[i] = pos_actuator_[i]->Calculate(
            angle, motor_[i]->GetSpeed(), angle_[i], dt);
        motor_[i]->Control(out_[i]);
      }
    } else {
      bool reach = true;

      for (int i = 0; i < Num; i++) {
        if (!limit_check_[i]->ReachLimit()) {
          reach = false;
          this->out_[i] = pos_actuator_[i]->SpeedCalculate(
              this->param_.cali_speed, motor_[i]->GetSpeed(), dt);
          motor_[i]->Control(out_[i]);
        } else {
          this->out_[i] =
              pos_actuator_[i]->SpeedCalculate(0.0f, motor_[i]->GetSpeed(), dt);
          motor_[i]->Control(out_[i]);
        }
      }

      if (reach) {
        this->need_cali_ = false;

        for (int i = 0; i < Num; i++) {
          this->angle_[i] = param_.zero_angle;
          this->motor_[i]->Relax();
        }
      }
    }

    last_control_time_ = bsp_time_get();
  }

  void Relax() {
    for (auto motor : motor_) {
      motor->Relax();
    }
  }

  void ControlSpeed(float speed, float dt) {
    for (int i = 0; i < Num; i++) {
      pos_actuator_[i]->SpeedCalculate(speed, motor_[i]->GetSpeed(), dt);
    }
  }

  bool Ready() { return !need_cali_; }

  friend PosStream& operator>>(PosStream& stream,
                               SteeringMech<MotorType, LimitType, Num>& mech) {
    float error = 0.0f, setpoint = 0.0f;

    if (!mech.Ready()) {
      mech.Relax();
      return stream;
    }

    switch (mech.param_.axis) {
      case AXIS_X:
        error = stream.target_angle_.pit - stream.angle_.pit;
        break;
      case AXIS_Y:
        error = stream.target_angle_.rol - stream.angle_.rol;
        break;
      case AXIS_Z:
        error = stream.target_angle_.yaw - stream.angle_.yaw;
        break;
      default:
        XB_ASSERT(false);
        break;
    }

    if (error < mech.param_.min_angle) {
      error -= mech.param_.min_angle;
      setpoint = mech.param_.min_angle;
    } else if (error > mech.param_.max_angle) {
      error -= mech.param_.max_angle;
      setpoint = mech.param_.max_angle;
    } else {
      setpoint = error;
      error = 0;
    }

    switch (mech.param_.axis) {
      case AXIS_X:
        stream.angle_.pit += setpoint;
        break;
      case AXIS_Y:
        stream.angle_.rol += setpoint;
        break;
      case AXIS_Z:
        stream.angle_.yaw += setpoint;
        break;
      default:
        XB_ASSERT(false);
        break;
    }

    mech.Control(setpoint, TIME_DIFF(mech.last_control_time_, bsp_time_get()));

    Component::Type::Vector3 transform = mech.param_.translation;

    Component::Trans::EulrPosTrans(stream.angle_, transform);

    stream.pos_.x += transform.x;
    stream.pos_.y += transform.y;
    stream.pos_.z += transform.z;

    return stream;
  }

  static PosStream& GroupControl(
      PosStream& stream, SteeringMech<MotorType, LimitType, Num>& mech_1,
      SteeringMech<MotorType, LimitType, Num>& mech_2) {
    if (mech_1.param_.axis != mech_2.param_.axis) {
      XB_ASSERT(false);
    }

    if (!mech_1.Ready() || !mech_2.Ready()) {
      mech_1.Relax();
      mech_2.Relax();
      return stream;
    }

    float error = 0.0f, setpoint_1 = 0.0f, setpoint_2 = 0.0f;

    switch (mech_1.param_.axis) {
      case AXIS_X:
        error = stream.target_angle_.pit - stream.angle_.pit;
        break;
      case AXIS_Y:
        error = stream.target_angle_.rol - stream.angle_.rol;
        break;
      case AXIS_Z:
        error = stream.target_angle_.yaw - stream.angle_.yaw;
        break;
      default:
        XB_ASSERT(false);
        break;
    }

    if (error < mech_2.param_.min_angle) {
      setpoint_2 = mech_2.param_.min_angle;
      setpoint_1 = error - setpoint_2;
    } else if (error > mech_2.param_.max_angle) {
      setpoint_2 = mech_2.param_.max_angle;
      setpoint_1 = error - setpoint_2;
    } else {
      setpoint_2 = error;
      setpoint_1 = 0;
    }

    if (mech_1.param_.max_angle - setpoint_1 >
        -mech_2.param_.min_angle + setpoint_2) {
      error = -mech_2.param_.min_angle + setpoint_2;
    } else {
      error = mech_1.param_.max_angle - setpoint_1;
    }

    setpoint_1 += error;
    setpoint_2 -= error;
    mech_1.Control(setpoint_1,
                   TIME_DIFF(mech_1.last_control_time_, bsp_time_get()));
    mech_2.Control(setpoint_2,
                   TIME_DIFF(mech_2.last_control_time_, bsp_time_get()));

    switch (mech_1.param_.axis) {
      case AXIS_X: {
        stream.angle_.pit += setpoint_2;
        Component::Type::Vector3 transform = mech_1.param_.translation;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        transform = mech_2.param_.translation;
        stream.angle_.pit += setpoint_1;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        break;
      }
      case AXIS_Y: {
        stream.angle_.rol += setpoint_2;
        Component::Type::Vector3 transform = mech_1.param_.translation;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        transform = mech_2.param_.translation;
        stream.angle_.rol += setpoint_1;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        break;
      }
      case AXIS_Z: {
        stream.angle_.yaw += setpoint_2;
        Component::Type::Vector3 transform = mech_1.param_.translation;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        transform = mech_2.param_.translation;
        stream.angle_.yaw += setpoint_1;
        Component::Trans::EulrPosTrans(stream.angle_, transform);
        stream.pos_.x += transform.x;
        stream.pos_.y += transform.y;
        stream.pos_.z += transform.z;
        break;
      }
      default:
        XB_ASSERT(false);
        break;
    }

    return stream;
  }

 private:
  bool need_cali_ = true;
  std::array<float, Num> angle_;
  std::array<float, Num> out_;
  std::array<Component::Type::CycleValue, Num> last_motor_pos_;
  std::array<BaseMotor*, Num> motor_;

  Param& param_;
  std::array<Component::PosActuator*, Num> pos_actuator_;
  std::array<LimitType*, Num> limit_check_;
  uint64_t last_control_time_ = 0;
};

template <typename MotorType, typename LimitType, int Num>
class LinearMech {
 public:
  typedef struct {
    std::array<typename LimitType::Param, Num> limit_param;
    std::array<Component::PosActuator::Param, Num> pos_actuator;
    std::array<typename MotorType::Param, Num> motor_param;
    std::array<const char*, Num> motor_name;
    float cali_speed;      /* 校准时的电机运动速度 */
    float max_distance;    /* 位移最大值 */
    float margin_error;    /* 正常运动时距离边界的最小距离 */
    float zero_position;   /* 触发限位时的位置 */
    float reduction_ratio; /* 总减速比 */
    /* 位移为0时下级机构转动轴/零点距离当前机构零点的坐标 */
    Component::Type::Vector3 translation;
    Axis axis; /* 运动方向 */
  } Param;

  LinearMech(Param& param, float control_freq) : param_(param) {
    for (int i = 0; i < Num; i++) {
      this->motor_[i] =
          new MotorType(param.motor_param[i], param_.motor_name[i]);
      this->pos_actuator_[i] =
          new Component::PosActuator(param.pos_actuator[i], control_freq);
      this->limit_check_[i] = new LimitType(param.limit_param[i]);
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

  void Control(float postion, float dt) {
    if (!need_cali_) {
      clampf(&postion, param_.margin_error,
             param_.max_distance - param_.margin_error);

      for (int i = 0; i < Num; i++) {
        this->out_[i] = pos_actuator_[i]->Calculate(
            postion, motor_[i]->GetSpeed(), position_[i], dt);
        motor_[i]->Control(out_[i]);
      }
    } else {
      bool reach = true;

      for (int i = 0; i < Num; i++) {
        if (!limit_check_[i]->ReachLimit()) {
          reach = false;
          this->out_[i] = pos_actuator_[i]->SpeedCalculate(
              this->param_.cali_speed, motor_[i]->GetSpeed(), dt);
          motor_[i]->Control(out_[i]);
        } else {
          this->out_[i] =
              pos_actuator_[i]->SpeedCalculate(0.0f, motor_[i]->GetSpeed(), dt);
          motor_[i]->Control(out_[i]);
        }
      }

      if (reach) {
        this->need_cali_ = false;

        for (int i = 0; i < Num; i++) {
          this->position_[i] = param_.zero_position;
          this->motor_[i]->Relax();
        }
      }
    }

    last_control_time_ = bsp_time_get();
  }

  void Relax() {
    for (auto motor : motor_) {
      motor->Relax();
    }
  }

  void ControlSpeed(float speed, float dt) {
    for (int i = 0; i < Num; i++) {
      pos_actuator_[i]->SpeedCalculate(speed, motor_[i]->GetSpeed(), dt);
    }
  }

  bool Ready() { return !need_cali_; }

  friend PosStream& operator>>(PosStream& stream,
                               LinearMech<MotorType, LimitType, Num>& mech) {
    float error = 0.0f, setpoint = 0.0f;

    if (!mech.Ready()) {
      mech.Relax();
      return stream;
    }

    switch (mech.param_.axis) {
      case AXIS_X:
        error = stream.target_pos_.x - stream.pos_.x;
        break;
      case AXIS_Y:
        error = stream.target_pos_.y - stream.pos_.y;
        break;
      case AXIS_Z:
        error = stream.target_pos_.z - stream.pos_.z;
        break;
      default:
        XB_ASSERT(false);
        break;
    }

    if (error < 0) {
      setpoint = 0;
    } else if (error > mech.param_.max_distance) {
      error -= mech.param_.max_distance;
      setpoint = mech.param_.max_distance;
    } else {
      setpoint = error;
      error = 0;
    }

    switch (mech.param_.axis) {
      case AXIS_X:
        stream.pos_.x += setpoint;
        break;
      case AXIS_Y:
        stream.pos_.y += setpoint;
        break;
      case AXIS_Z:
        stream.pos_.z += setpoint;
        break;
      default:
        XB_ASSERT(false);
        break;
    }

    mech.Control(setpoint, TIME_DIFF(mech.last_control_time_, bsp_time_get()));

    return stream;
  }

 private:
  bool need_cali_ = true;
  std::array<float, Num> position_;
  std::array<float, Num> out_;
  std::array<Component::Type::CycleValue, Num> last_motor_pos_;
  std::array<BaseMotor*, Num> motor_;

  Param& param_;
  std::array<Component::PosActuator*, Num> pos_actuator_;
  std::array<LimitType*, Num> limit_check_;
  uint64_t last_control_time_ = 0;
};
}  // namespace Device
