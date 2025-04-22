#include "dev_ahrs.hpp"
#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "dev_icm42688.hpp"
#include "dev_mmc5603.hpp"
#include "mod_canfd_imu.hpp"

void robot_init();
namespace Robot {
class CanfdIMU {
 public:
  typedef struct Param {
    Device::MMC5603::Rotation mmc5603_rot{};
    Device::ICM42688::Rotation icm42688_rot{};
    Device::BlinkLED::Param led{};
  } Param;

  Device::Can can_;
  Device::AHRS ahrs_;
  Device::ICM42688 icm42688_;
  Device::MMC5603 mmc5603_;
  Device::BlinkLED led_;
  Module::CanfdImu imu_;

  CanfdIMU(Param& param)
      : icm42688_(param.icm42688_rot),
        mmc5603_(param.mmc5603_rot, 0.03),
        led_(param.led) {}
};
}  // namespace Robot
