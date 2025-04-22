#include <database.hpp>

#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_ai.hpp"
#include "dev_blink_led.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_can_imu.hpp"
#include "dev_cap.hpp"
#include "dev_dr16.hpp"
#include "dev_referee.hpp"
#include "mod_balance.hpp"
#include "mod_dual_leg.hpp"
#include "mod_gimbal.hpp"
#include "mod_launcher.hpp"

void robot_init();
namespace Robot {
class Infantry {
 public:
  typedef struct Param {
    Module::RMDBalance::Param balance{};
    Module::WheelLeg::Param leg{};
    Module::Launcher::Param launcher;
    Module::Gimbal::Param gimbal;
    Device::BMI088::Rotation bmi088_rot{};
    Device::Cap::Param cap{};
    Device::IMU::Param can_imu{};
    Device::BlinkLED::Param blink{};
  } Param;

  Component::CMD cmd_;

  Device::AI ai_;
  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::Can can_;
  Device::IMU can_imu_;
  Device::Referee referee_;
  Device::Cap cap_;
  Device::DR16 dr16_;
  Device::BlinkLED led_;

  Module::WheelLeg leg_;
  Module::RMDBalance balance_;
  Module::Gimbal gimbal_;
  Module::Launcher launcher_;

  static Infantry* self_;

  Infantry(Param& param, float control_freq)
      : bmi088_(param.bmi088_rot),
        can_imu_(param.can_imu),
        cap_(param.cap),
        led_(param.blink),
        leg_(param.leg, control_freq),
        balance_(param.balance, control_freq),
        gimbal_(param.gimbal, control_freq),
        launcher_(param.launcher, control_freq) {
    self_ = this;
  }
};
}  // namespace Robot
