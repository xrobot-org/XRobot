
#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "dev_referee.hpp"
#include "mod_gimbal.hpp"
#include "mod_launcher_drone.hpp"

void robot_init();

namespace Robot {
class UVA {
 public:
  typedef struct {
    Module::Gimbal::Param gimbal;
    Module::UVALauncher::Param launcher;
    Device::BMI088::Rotation bmi088_rot;
  } Param;

  Component::CMD cmd_;

  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::Can can_;
  Device::DR16 dr16_;
  Device::RGB led_;
  Device::Referee referee_;

  Module::Gimbal gimbal_;
  Module::UVALauncher launcher_;
  static UVA* self_;
  UVA(Param& param, float control_freq)
      : bmi088_(param.bmi088_rot),
        gimbal_(param.gimbal, control_freq),
        launcher_(param.launcher, control_freq) {
    self_ = this;
  }
};
}  // namespace Robot
