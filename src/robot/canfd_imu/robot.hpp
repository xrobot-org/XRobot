#include "dev_ahrs.hpp"
#include "dev_blink_led.hpp"
#include "dev_canfd.hpp"
#include "dev_icm42688.hpp"
#include "dev_mmc5603.hpp"
#include "mod_canfd_imu.hpp"

void robot_init();
namespace Robot {
class WearLabIMU {
 public:
  typedef struct Param {
    Device::ICM42688::Rotation imu_rot{};
    Device::MMC5603::Rotation magn_rot{};
    Device::BlinkLED::Param led{};
  } Param;

  Device::AHRS ahrs_;
  Device::ICM42688 icm42688_;
  Device::BlinkLED led_;
  Device::Can can_;
  Device::MMC5603 mmc5603_;

  Module::CanfdImu canfd_imu_;

  WearLabIMU(Param& param)
      : icm42688_(param.imu_rot), led_(param.led), mmc5603_(param.magn_rot) {}
};
}  // namespace Robot
