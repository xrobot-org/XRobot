#include <comp_cmd.hpp>

#include "dev_ahrs.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "dev_referee.hpp"
#include "mod_dart_launcher.hpp"
void robot_init();

namespace Robot {
class Dart {
 public:
  typedef struct Param {
    Module::DartLauncher::Param dart{};
    Device::BMI088::Rotation bmi088{};
  } Param;

  Component::CMD cmd_;
  Device::RGB rgb_{};
  Device::Referee referee;
  Device::Can can_;
  Device::DR16 dr16_;
  Device::BMI088 bmi088_;
  Device::AHRS ahrs_;

  Module::DartLauncher dartlauncher_;

  Dart(Param& param, float control_freq)
      : bmi088_(param.bmi088), dartlauncher_(param.dart, control_freq) {}
};
}  // namespace Robot
