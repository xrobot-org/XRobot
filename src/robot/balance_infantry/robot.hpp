#include <database.hpp>

#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_ai.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_can_imu.hpp"
#include "dev_cap.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "dev_referee.hpp"
#include "mod_balance.hpp"
#include "mod_dual_leg.hpp"

void robot_init();
namespace Robot {
class Infantry {
 public:
  typedef struct {
    Module::RMDBalance::Param balance;
    Module::WheelLeg::Param leg;
    Device::BMI088::Rotation bmi088_rot;
    Device::Cap::Param cap;
    Device::IMU::Param can_imu;
  } Param;

  Component::CMD cmd_;

  Device::AI ai_;
  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::Can can_;
  Device::IMU can_imu_;
  Device::Cap cap_;
  Device::DR16 dr16_;
  Device::RGB led_;
  Device::Referee referee_;

  Module::WheelLeg leg_;
  Module::RMDBalance balance_;
  // Module::Gimbal gimbal_;
  // Module::Launcher launcher_;

  Infantry(Param& param, float control_freq)
      : bmi088_(param.bmi088_rot),
        can_imu_(param.can_imu),
        cap_(param.cap),
        leg_(param.leg, control_freq),
        balance_(param.balance, control_freq) {}
};
}  // namespace Robot
