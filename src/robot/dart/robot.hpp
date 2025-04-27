/* #include "dev_xxx.hpp" */
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
  typedef struct Param {
    Module::Dartgimbal::Param gimbal{};
    Module::DartLauncher::Param launcher{};
    Device::BMI088::Rotation bmi088{};
  } Param;

  Component::CMD cmd_;
  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::Can can_;
  Device::DR16 dr16_;
  Device::RGB led_;

  Module::DartLauncher dartlauncher_;
  Module::Dartgimbal dartgimbal_;

  Dart(Param& param, float control_freq)
      : bmi088_(param.bmi088),
        dartlauncher_(param.launcher, control_freq),
        dartgimbal_(param.gimbal, control_freq) {}
};
}  // namespace Robot
