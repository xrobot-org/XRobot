#include "dev_ahrs.hpp"
#include "dev_blink_led.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_icm42688.hpp"
#include "mod_canfd_imu.hpp"

void robot_init();
namespace Robot {
class CanfdIMU {
 public:
  typedef struct Param {
    Device::ICM42688::Rotation icm42688_rot{};
    Device::BMI088::Rotation bmi088_rot{};
    Device::BlinkLED::Param led{};
  } Param;

  Device::Can can_;
  Device::AHRS ahrs_;
  Device::ICM42688 icm42688_;
  Device::BMI088 bmi088_;
  Device::BlinkLED led_;
  Module::CanfdImu imu_;

  CanfdIMU(Param& param)
      : icm42688_(param.icm42688_rot),
        bmi088_(param.bmi088_rot),
        led_(param.led) {}
};
}  // namespace Robot
