#include "robot.hpp"

#include <comp_actuator.hpp>

#include "dev_rm_motor.hpp"
#include "mod_Radar.hpp"
#include "system.hpp"

/* clang-format off */
Robot::Radar_car::Param param = {
    .chassis={
      .toque_coefficient_ = 0.0327120418848f,
      .speed_2_coefficient_ = 2.300974248103511e-07f,
      .out_2_coefficient_ = 2.1455244766462685e-29f,
      .constant_ = 0.23958431845825284f,
      .type = Component::Mixer::OMNICROSS,

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Radar_car::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::Radar_car::SET_MODE_INDENPENDENT
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::Radar_car::SET_MODE_INDENPENDENT
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_BOT,
        Module::Radar_car::SET_MODE_INDENPENDENT
      },
       Component::CMD::EventMapItem{
        Device::AI::AIControlData::AI_ROTOR,
        Module::Radar_car::SET_MODE_INDENPENDENT
      }
    },

    .actuator_param = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00018f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .motor_param = {
      Device::RMMotor::Param{
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x204,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
    },
    // .get_speed = [](float power_limit){
    //   float speed = 0.0f;
    //  if (power_limit <= 50.0f) {
    //   speed = 0.0f;
    //   } else if (power_limit <= 60.0f) {
    //     speed = 3800;
    //   } else if (power_limit <= 70.0f) {
    //     speed = 5000;
    //   } else if (power_limit <= 80.0f) {
    //     speed = 5500;
    //   } else if (power_limit <= 100.0f) {
    //     speed = 6000;
    //   } else {
    //     speed = 6500;
    //   }
    //   return speed;
    // },
      }

 };
void robot_init() {
  System::Start<Robot::Radar_car, Robot::Radar_car::Param>(param, 500.0f);
}
