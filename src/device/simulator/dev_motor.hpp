#pragma once

#include <webots/motor.h>
#include <webots/position_sensor.h>
#include <webots/robot.h>

#include <comp_type.hpp>
#include <device.hpp>

#include "bsp_time.h"

namespace Device {
class BaseMotor {
 public:
  typedef struct {
    float rotor_abs_angle;  /* 转子绝对角度 单位：rad */
    float rotational_speed; /* 转速 单位：rpm */
    float torque_current;   /* 转矩电流 单位：A*/
    float temp;             /* 电机温度 单位：℃*/
  } Feedback;

  BaseMotor(const char *name)
      : handle_(wb_robot_get_device(name)),
        cmd_(this, BaseMotor::ShowCMD, this->name_, System::Term::DevDir()) {
    strncpy(this->name_, name, sizeof(this->name_));
    memset(&(this->feedback_), 0, sizeof(this->feedback_));
    this->sensor_ =
        wb_robot_get_device((std::string(name) + "_Sensor").c_str());
    wb_motor_set_position(this->handle_, INFINITY);
    wb_motor_set_velocity(this->handle_, 0.0f);
    wb_position_sensor_enable(this->sensor_, 1);
  }

  virtual void Control(float output) = 0;

  virtual bool Update() = 0;

  void Relax() { wb_motor_set_torque(this->handle_, 0.0f); }

  float GetAngle() { return this->feedback_.rotor_abs_angle; }

  float GetSpeed() { return this->feedback_.rotational_speed; }

  float GetCurrent() { return this->feedback_.torque_current; }

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
          printf("最近一次反馈时间:%fs.\r\n", motor->last_online_time_);
          printf("角度:%frad 速度:%frpm 电流:%fA 温度:%f℃\r\n",
                 motor->feedback_.rotor_abs_angle,
                 motor->feedback_.rotational_speed,
                 motor->feedback_.torque_current, motor->feedback_.temp);
          System::Thread::Sleep(delay);
          ms_clear_line();
          time -= delay;
        }
      }
    }
    return 0;
  }

  char name_[20]{};

  Feedback feedback_{};

  uint32_t last_online_time_ = 0;

  Component::Type::CycleValue last_pos_ = 0.0f;

  uint32_t last_sensor_time_ = 0;

  WbDeviceTag handle_;

  WbDeviceTag sensor_;

  System::Term::Command<BaseMotor *> cmd_;
};
}  // namespace Device
