#include <comp_actuator.hpp>
#include <comp_vmc.hpp>
#include <comp_pid.hpp>

#include <system.hpp>
#include <thread.hpp>

#include "dev_bmi088.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "dev_mit_motor.hpp"
#include "comp_cmd.hpp"
#include "dev_ahrs.hpp"
#include "dev_referee.hpp"

#include "mod_dm_balance.hpp"

void robot_init();

namespace Robot
{
class DmRobot
{
 public:
  typedef struct Param {
    Module::Balance::Param chassis;

    Device::BMI088::Rotation bmi088_rot{};
   // Device::Cap::Param cap{};
  } Param;
 Component::CMD cmd_;
 Device::Can can_;
 Device::AHRS ahrs_;
 Device::BMI088 bmi088_;
 Device::DR16 dr16_;

 Module::Balance chassis_;

DmRobot(Param& param)
      :
        //cap_(param.cap),
        bmi088_(param.bmi088_rot),
        chassis_(param.chassis) {}



};
}  // namespace Robot
