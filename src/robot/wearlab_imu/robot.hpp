#include "dev_ahrs.hpp"
#include "dev_blink_led.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "mod_can_imu.hpp"

void robot_init();
namespace Robot {
class WearLabIMU {
 public:
  typedef struct Param {
    Device::BMI088::Rotation bmi088_rot{};
    Device::BlinkLED::Param led{};
  } Param;

  Device::AHRS ahrs_;
  Device::BMI088 bmi088_;
  Device::BlinkLED led_;
  Device::Can can_;
  Module::CanIMU imu_;

  WearLabIMU(Param& param) : bmi088_(param.bmi088_rot), led_(param.led) {}
};
}  // namespace Robot
