#include "comp_ahrs.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_led.hpp"
#include "mod_can_imu.hpp"

void robot_init();
namespace Robot {
class Infantry : public System::Message {
 public:
  typedef struct {
    Device::BMI088::Rotation bmi088_rot;
    Device::BMI088::Calibration bmi088_cali;
  } Param;

  Component::AHRS ahrs_;
  Device::CAN can_;
  Device::BMI088 bmi088_;
  Device::LED led_;
  Module::CanIMU imu_;

  Infantry(Param& param) : bmi088_(param.bmi088_cali, param.bmi088_rot) {}
};
}  // namespace Robot
