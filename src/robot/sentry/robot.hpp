#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_ai.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_cap.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "dev_referee.hpp"
#include "mod_chassis.hpp"
#include "mod_gimbal.hpp"
#include "mod_launcher.hpp"

void robot_init();
namespace Robot {
class Sentry {
 public:
  typedef struct Param {
    Module::RMChassis::Param chassis;
    Module::Gimbal::Param gimbal;
    Module::Launcher::Param launcher1;
    Module::Launcher::Param launcher2;
    Device::BMI088::Rotation bmi088_rot{};
    Device::Cap::Param cap{};
  } Param;

  Component::CMD cmd_;

  Device::AI ai_;
  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::Can can_;
  Device::Cap cap_;
  Device::DR16 dr16_;
  Device::RGB led_;
  Device::Referee referee_;

  Module::RMChassis chassis_;
  Module::Gimbal gimbal_;
  Module::Launcher launcher1_;
  Module::Launcher launcher2_;
  Sentry(Param& param, float control_freq)
      : cmd_(Component::CMD::CMD_OP_CTRL),
        bmi088_(param.bmi088_rot),
        cap_(param.cap),
        chassis_(param.chassis, control_freq),
        gimbal_(param.gimbal, control_freq),
        launcher1_(param.launcher1, control_freq),
        launcher2_(param.launcher2, control_freq) {}
};
}  // namespace Robot
