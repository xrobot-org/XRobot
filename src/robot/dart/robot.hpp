#include <comp_cmd.hpp>

#include "dev_ahrs.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "mod_dart_gimbal.hpp"
#include "mod_dart_launcher.hpp"
void robot_init();

namespace Robot {
class Dart {
 public:
  typedef struct {
    Module::DartLauncher::Param dart;
    Module::Dartgimbal::Param gimbal;
    Device::BMI088::Rotation bmi088;
  } Param;

  Component::CMD cmd_;
  Device::RGB rgb_;
  Device::Can can_;
  Device::DR16 dr16_;
  Device::BMI088 bmi088_;
  Device::AHRS ahrs_;

  // Module::DartLauncher dart_;
  Module::Dartgimbal gimbal_;

  static Dart* self_;

  Dart(Param& param, float control_freq)
      : bmi088_(param.bmi088), gimbal_(param.gimbal, control_freq) {
    self_ = this;
  }
};
}  // namespace Robot
