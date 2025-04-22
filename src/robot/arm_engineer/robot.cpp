#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

//          p    i    d           k
// yaw2     60   0.1  0.3         1
// roll1    139  0.3  4.6         1
// roll2_pos 100   0   0.1         10
// yaw1 dong bu liao

using namespace Robot;

/* clang-format off */
Robot::ArmEngineer::Param param = {
  .chassis = {
      .type = Component::Mixer::MECANUM,


      .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::RMChassis::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_TOP,
          Module::RMChassis::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_MID,
          Module::RMChassis::SET_MODE_INDENPENDENT
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_F,//底盘不会动
          Module::RMChassis::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
        Device::DR16::KEY_G,
        Module::RMChassis::SET_MODE_INDENPENDENT//底盘跟随
      },
      },

      .actuator_param = {
        Component::SpeedActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.15f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.15f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.15f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.15f,
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
            .can = BSP_CAN_3,
            .reverse = 0

        },
        Device::RMMotor::Param{
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_3,
            .reverse = 0
        },
        Device::RMMotor::Param{
            .id_feedback = 0x203,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_3,
            .reverse = 0
        },
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_3,
            .reverse = 0
        }
    },
  },


  .robotarm = {

  .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::RobotArm::SET_MODE_RELAX
        },                              
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_R_POS_TOP,
          Module::RobotArm::SET_MODE_WORK_TOP
        },
      
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_R_POS_MID,
          Module::RobotArm::SET_MODE_CUSTOM_CTRL
        },
    
       /***ASDW,不要使用. CTRL+SHIFT+Q和E组合用上了..
    KEY_Q,
    KEY_E,
    KEY_R,
    KEY_F,
    KEY_G,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_L_PRESS,
    KEY_R_PRESS,
    KEY_L_RELEASE,
    KEY_R_RELEASE,*/
     Component::CMD::EventMapItem{
          Device::DR16::KEY_Q,
          Module::RobotArm::SET_MODE_WORK_TOP
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_E,
         Module::RobotArm::SET_MODE_YINKUANG
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_R,
          Module::RobotArm::SET_MODE_SKUANG
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_X,
          Module::RobotArm::SET_MODE_XIKUANG
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_C,
          Module::RobotArm::SET_MODE_SAVE1
        },
         Component::CMD::EventMapItem{
        Device::DR16::KEY_B,
        Module::RobotArm::SET_MODE_QKONE
      },
  },
.yaw1_actr={
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 1.0f,//
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.0f,
          .p = 3.0f,
          .i = 0.0f,
          .d = 0.4f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
.yaw2_actr={//ruan
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.0f,
          .p = 20.0f,
          .i = 0.0f,
          .d = 0.3f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
  .pitch1_actr={
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 0.4f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.0f,
          .p = 250.0f,
          .i = 0.1f,
          .d = 10.0f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
  .pitch2_actr={
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 0.4f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.0f,
          .p = 450.0f,//450
          .i = 0.2f,
          .d = 15.0f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
  .roll1_actr={//ruan//
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 0.2f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.0f,
          .p = 5.1f,
          .i = 0.0f,
          .d = 0.4f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
.roll2_actr={
        .speed = {
          .k = 0.1f,//0.05
          .p = 0.025f,//0.05
          .i = 0.2f,//0
          .d = 0.0f,//0
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 10.0f,
          .p = 6.0f,
          .i = 0.0f,
          .d = 0.3f,
          .i_limit = 2.0f,
          .out_limit = 200.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
  },
.yaw1_motor={//最下面的大yaw
  .kp=15.0f,//15//3
  .kd=0.3f,
  .feedback_id = 0x01,//1
  .id = 0x01,
  .can = BSP_CAN_3,//3
  .reverse = false,
},
.yaw2_motor={//传带的那个4310
  .kp=10.0f,//10
  .kd=0.5f,
  .feedback_id = 0x05,//05
  .id = 0x05,
  .can = BSP_CAN_1,
  .reverse = false,
},


.pitch1_motor={
  .kp=100.0f,//100
  .kd=0.6f,
  .feedback_id = 0x02,
  .id = 0x02,
  .can = BSP_CAN_2,
  .reverse = false,
},
.pitch2_motor={
  .kp=70.0f,//150
  .kd=0.6f,
  .feedback_id = 0x03,
  .id = 0x03,
  .can = BSP_CAN_2,
  .reverse = true,//pitch2反转//这个现在之影响读角度了。
},

.roll1_motor={//末端倒数第二个电机
  .kp=10.0f,//20
  .kd=0.3f,
  .feedback_id = 0x06,//06
  .id = 0x06,
  .can = BSP_CAN_1,
  .reverse = false,
},
.roll2_motor={
        .id_feedback = 0x208,//0x208
        .id_control = GM6020_CTRL_ID_BASE,
        .model = Device::RMMotor::MOTOR_GM6020,
        .can = BSP_CAN_1,
        .reverse = 0
},



.limit = {
          //看清楚大和小的位置，确定上下限，不要天翻了
        .yaw1_max = 3.10f,
        .yaw1_min =-3.10f,//3.40
        //pitch1  8009I
        .pitch1_max = 3.14f,
        .pitch1_min = -2.80f,
        //pitch2  8009II
        .pitch2_max =2.91f,//2现在是最低段，
        .pitch2_min =0.0f,//-3.14f,
        //
        // .yaw2_max = 6.00f,
        // .yaw2_min = -6.00f,
        //roll1  //末端倒数第二个电机
        .roll1_max=1.65f,
        .roll1_min=-1.50f,
      },


}





};
/* clang-format on */

void robot_init() {
  // System::Start<Robot::ArmEngineer, Robot::ArmEngineer::Param>(param);
  System::Start<Robot::ArmEngineer, Robot::ArmEngineer::Param>(param, 500.0f);
}
