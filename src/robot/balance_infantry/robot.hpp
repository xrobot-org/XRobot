#include "comp_ahrs.hpp"
#include "comp_cmd.hpp"
#include "dev_ai.hpp"
#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_can_imu.hpp"
#include "dev_cap.hpp"
#include "dev_dr16.hpp"
#include "dev_led.hpp"
#include "dev_referee.hpp"
#include "dev_term.hpp"
#include "mod_balance.hpp"
#include "mod_dual_leg.hpp"

void robot_init();
namespace Robot {
class Infantry : public System::Message {
 public:
  typedef struct {
    Module::RMBalance::Param balance;
    Module::WheelLeg::Param leg;
    Device::BMI088::Rotation bmi088_rot;
    Device::BMI088::Calibration bmi088_cali;
    Device::Cap::Param cap;
    Device::IMU::Param can_imu;
  } Param;

  Component::CMD cmd_;
  Component::AHRS ahrs_;

  Device::AI ai_;
  Device::BMI088 bmi088_;
  Device::CAN can_;
  Device::IMU can_imu_;
  Device::Cap cap_;
  Device::DR16 dr16_;
  Device::LED led_;
  Device::Referee referee_;
  Device::Term term_;

  Module::WheelLeg leg_;
  Module::RMBalance balance_;
  // Module::Gimbal gimbal_;
  // Module::Launcher launcher_;

  Infantry(Param& param, float control_freq)
      : bmi088_(param.bmi088_cali, param.bmi088_rot),
        can_imu_(param.can_imu),
        cap_(param.cap),
        leg_(param.leg, control_freq),
        balance_(param.balance, control_freq) {}
};
}  // namespace Robot
