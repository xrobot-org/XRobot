#include "comp_ahrs.hpp"
#include "database.hpp"
#include "dev_blink_led.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "mod_can_imu.hpp"
#include "term.hpp"

void robot_init();
namespace Robot {
class Infantry {
 public:
  typedef struct {
    Device::BMI088::Rotation bmi088_rot;
    Device::BlinkLED::Param led_;
  } Param;

  Message message_;

  System::Term term_;
  System::Database database_;

  Component::AHRS ahrs_;

  Device::BMI088 bmi088_;
  Device::BlinkLED led_;
  Device::Can can_;
  Module::CanIMU imu_;

  Infantry(Param& param) : bmi088_(param.bmi088_rot), led_(param.led_) {}
};
}  // namespace Robot
