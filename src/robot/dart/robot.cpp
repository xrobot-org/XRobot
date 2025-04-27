#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

using namespace Robot;

/* clang-format off */
//TODO: write your param
Dart::Param param={
  .gimbal={
    .yaw_param={
      .stall_detect={
        .speed_thld=500.0f,
        .current_thld=1.2f,
        .stop_current_thld=2.5f,
        .temp_thld=40.0f,
        .timeout=0.1f
      },
      .pos_actuator={
        Component::PosActuator::Param{
          .speed={
            .k=0.00005f,
            .p=1.0f,
            .i=0.8f,
            .d=0.0f,
            .i_limit=0.5f,
            .out_limit=0.5f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .position={
            .k=2000.0f,
            .p=0.8f,
            .i=0.0f,
            .d=0.0f,
            .i_limit=4000.0f,
            .out_limit=8000.0f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .in_cutoff_freq=10.0f,
          .out_cutoff_freq=30.0f,
        },
      },
      .motor_param={
        Device::RMMotor::Param{
          .id_feedback=0x207,
          .id_control=M3508_M2006_CTRL_ID_EXTAND,
          .model=Device::RMMotor::MOTOR_M2006,
          .can=BSP_CAN_1,
          .reverse=false,
        },
      },
      .motor_name={
        "yaw"
      },
      .cali_speed=-2000.0f,
      .max_range=250.0f,
      .margin_error=3.0f,
      .reduction_ratio=3591.0f/187.0f,
    },

    .pitch_param={
      .stall_detect={
        .speed_thld=500.0f,
        .current_thld=1.2f,
        .stop_current_thld=2.5f,
        .temp_thld=40.0f,
        .timeout=0.1f
      },
      .pos_actuator={
        Component::PosActuator::Param{
          .speed={
            .k=0.00005f,
            .p=1.0f,
            .i=0.8f,
            .d=0.0f,
            .i_limit=0.5f,
            .out_limit=0.5f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .position={
            .k=2000.0f,
            .p=1.0f,
            .i=0.0f,
            .d=0.0f,
            .i_limit=4000.0f,
            .out_limit=8000.0f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .in_cutoff_freq=10.0f,
          .out_cutoff_freq=30.0f,
        },
      },
      .motor_param={
        Device::RMMotor::Param{
          .id_feedback=0x206,
          .id_control=M3508_M2006_CTRL_ID_EXTAND,
          .model=Device::RMMotor::MOTOR_M3508,
          .can=BSP_CAN_1,
          .reverse=false,
        },
      },
      .motor_name={
        "pitch"
      },
      .cali_speed=-2000.0f,
      .max_range=400.0f,
      .margin_error=3.0f,
      .reduction_ratio=3591.0f/187.0f
    },
    .EVENT_MAP={
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Dartgimbal::SET_MODE_RELAX,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::Dartgimbal::SET_MODE_RELAX,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::Dartgimbal::SET_MODE_STABLE,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_BOT,
        Module::Dartgimbal::SET_MODE_CONTROL,
      },
    },
  },
  .launcher={
    .EVENT_MAP={
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::DartLauncher::SET_MODE_RELAX,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::DartLauncher::SET_MODE_OFF,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::DartLauncher::SET_MODE_STAY,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::DartLauncher::SET_MODE_ADVANCE,
      },
    },
    .rod_={
      .stall_detect={
        .speed_thld=500.0f,
        .current_thld=1.2f,
        .stop_current_thld=2.5f,
        .temp_thld=40.0f,
        .timeout=0.1f
      },
      .pos_actuator={
        Component::PosActuator::Param{
          .speed={
            .k=0.00005f,
            .p=1.0f,
            .i=0.8f,
            .d=0.0f,
            .i_limit=0.5f,
            .out_limit=0.5f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .position={
            .k=2000.0f,
            .p=1.0f,
            .i=0.6f,
            .d=0.0f,
            .i_limit=4000.0f,
            .out_limit=8000.0f,
            .d_cutoff_freq=-1.0f,
            .cycle=false,
          },
          .in_cutoff_freq=10.0f,
          .out_cutoff_freq=30.0f,
        },
      },
      .motor_param={
        Device::RMMotor::Param{
          .id_feedback=0x205,
          .id_control=M3508_M2006_CTRL_ID_EXTAND,
          .model=Device::RMMotor::MOTOR_M2006,
          .can=BSP_CAN_1,
          .reverse=false,
        },
      },
      .motor_name={
        "rod"
      },
      .cali_speed=-2000.0f,
      .max_range=750.0f,
      .margin_error=3.0f,
      .reduction_ratio=3591.0f/187.0f
    },
    .fric_actr = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.0005f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.0005f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.0005f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.0005f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      }
    },
    .fric_motor = {
        Device::RMMotor::Param{
            .id_feedback = 0x201,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = true,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = true,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x203,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = false,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = false,
        }
    }
  },
  .bmi088={
    .rot_mat={
      {+1,+0,+0},
      {+0,+1,+0},
      {+0,+0,+1},
    },
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Dart, Robot::Dart::Param>(param, 500.0f);
}
