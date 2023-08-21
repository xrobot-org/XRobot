#pragma once

#include <device.hpp>

namespace Device {
class BaseMotor {
 public:
  typedef struct Feedback {
    Component::Type::CycleValue rotor_abs_angle =
        0.0f;                      /* 转子绝对角度 单位：rad */
    float rotational_speed = 0.0f; /* 转速 单位：rpm */
    float torque_current = 0.0f;   /* 转矩电流 单位：A*/
    float temp = 0.0f;             /* 电机温度 单位：℃*/
  } Feedback;

  BaseMotor(const char *name, bool reverse)
      : reverse_(reverse),
        cmd_(this, BaseMotor::ShowCMD, this->name_, System::Term::DevDir()) {
    strncpy(this->name_, name, sizeof(this->name_));
    memset(&(this->feedback_), 0, sizeof(this->feedback_));
  }

  virtual void Control(float output) = 0;

  virtual bool Update() = 0;

  virtual void Relax() = 0;

  Component::Type::CycleValue GetAngle() {
    if (reverse_) {
      return -this->feedback_.rotor_abs_angle;
    } else {
      return this->feedback_.rotor_abs_angle;
    }
  }

  float GetSpeed() {
    if (reverse_) {
      return -this->feedback_.rotational_speed;
    } else {
      return this->feedback_.rotational_speed;
    }
  }

  float GetCurrent() {
    if (reverse_) {
      return -this->feedback_.torque_current;
    } else {
      return this->feedback_.torque_current;
    }
  }

  float GetTemp() { return this->feedback_.temp; }

  static int ShowCMD(BaseMotor *motor, int argc, char **argv) {
    if (argc == 1) {
      printf("[show] [time] [delay] 在time时间内每隔delay打印一次数据\r\n");
    } else if (argc == 4) {
      if (strcmp(argv[1], "show") == 0) {
        int time = std::stoi(argv[2]);
        int delay = std::stoi(argv[3]);

        if (delay > 1000) {
          delay = 1000;
        }
        if (delay < 2) {
          delay = 2;
        }
        while (time > delay) {
          printf("电机 [%s] 反馈数据:\r\n", motor->name_);
          printf("最近一次反馈时间:%fs.\r\n",
                 static_cast<float>(motor->last_online_time_) / 1000.0f);
          printf("角度:%frad 速度:%frpm 电流:%fA 温度:%f℃\r\n",
                 motor->GetAngle().Value(), motor->GetSpeed(),
                 motor->GetCurrent(), motor->feedback_.temp);
          System::Thread::Sleep(delay);
          ms_clear_line();
          time -= delay;
        }
      }
    }

    return 0;
  }

  char name_[25]{};

  Feedback feedback_;

  uint32_t last_online_time_ = 0;

  bool reverse_; /* 电机反装 */

  System::Term::Command<BaseMotor *> cmd_;
};
}  // namespace Device
