#include "robot.hpp"




#include <thread.hpp>

using namespace Robot;
Robot::DmRobot::Param param =
{
  .chassis =
  {
    .leg_param =
  {
    Component::VMC::Param{
      .leg_4 = 0.075f,
      .leg_1 = 0.075f,
      .leg_3 = 0.14f,
      .leg_2 = 0.14f,
      .hip_length = 0.08f,
    },

    Component::VMC::Param{
      .leg_4 = 0.075f,
      .leg_1 = 0.075f,
      .leg_3 = 0.14f,
      .leg_2 = 0.14f,
      .hip_length = 0.08f,
    },
  },
      .EVENT_MAP =

    {
      Component::CMD::EventMapItem
      {
      Component::CMD::CMD_EVENT_LOST_CTRL,
      Module::Balance::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem
      {
      Device::DR16::DR16_SW_L_POS_TOP,
      Module::Balance::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem
      {
      Device::DR16::DR16_SW_L_POS_MID,
      Module::Balance::SET_MODE_DATA
      },
      Component::CMD::EventMapItem
      {
      Device::DR16::DR16_SW_L_POS_BOT,
      Module::Balance::SET_MODE_STAND
      },

    },

  .mech_zero =
  {0.80777f, 0.0f ,-1.60735f,1.56233f},





  .leglength_pid_param =
  {
    Component::PID::Param{


  .k = 1.0f,
  .p = 500.0f,
  .i = 0.0f,
  .d = 50.0f,
  .i_limit = 1.0f,
 .out_limit = 100.0f,
 .d_cutoff_freq = -10.0f,
 .cycle = false,
},
    Component::PID::Param{


  .k = 1.0f,
  .p = 200.0f,
  .i = 0.0f,
  .d = 22.0f,
  .i_limit = 1.0f,
 .out_limit = 100.0f,
 .d_cutoff_freq = -1.f,
 .cycle = false,
}
  },

.Tp_pid_param =
{
 .k = 0.1f,
 .p = 1.0f,
 .i = 0.01f,
 .d = 0.4f,
 .i_limit = 1.0f,
 .out_limit = 5.0f,
 .d_cutoff_freq = -0.5f,
 .cycle = true,
},

.yaw_pid_param =
{
 .k = 0.1f,
 .p = 1.f,
 .i = 0.01f,
 .d = 0.2f,
 .i_limit = 1.0f,
 .out_limit = 1.0f,
 .d_cutoff_freq = -0.5f,
 .cycle = true,
},

  .hip_motor_param =
  {
    Device::MitMotor::Param
    {

    .kp = 0,
    .kd = 0,
    .feedback_id = 0x01,
    .id = 0x01,
    .can = BSP_CAN_2,
    .reverse = 1,

    },

    Device::MitMotor::Param
    {

    .kp = 0,
    .kd = 0,
    .feedback_id = 0x02,
    .id = 0x02,
    .can = BSP_CAN_2,
    .reverse = 1,

    },

    Device::MitMotor::Param
    {

    .kp = 0,
    .kd = 0,
    .feedback_id = 0x05,
    .id = 0x05,
    .can = BSP_CAN_1,
    .reverse = 1,

    },


    Device::MitMotor::Param
    {

    .kp = 0,
    .kd = 0,
    .feedback_id = 0x03,
    .id = 0x03,
    .can = BSP_CAN_2,
    .reverse = 1,

    },
  },



    .wheel_motor_param =
    {

  Device::RMMotor::Param
  {
  .id_feedback = 0x203,
  .id_control = M3508_M2006_CTRL_ID_BASE,
  .model = Device::RMMotor::MOTOR_M3508,
  .can = BSP_CAN_1,
  .reverse = 0,
 },

 Device::RMMotor::Param
 {
 .id_feedback = 0x204,
 .id_control = M3508_M2006_CTRL_ID_BASE,
 .model = Device::RMMotor::MOTOR_M3508,
 .can = BSP_CAN_1,
 .reverse = 0,
 },


    },


  .target_L0 =
  {
  0.055f,0.055f
  },
  .target_F =
  {
    4.0f,4.0f
  },


   .K_Poly_Coefficient_R =
   {

{-146.0942,78.8418,-17.4104,-0.0980},
{-7.7419,3.7325,-1.3242,-0.0118},
{-65.5165,31.5850,-5.0613,-0.1807},
{-39.4167,18.8614,-3.1689,-0.1688},
{-167.6425,99.9386,-21.5237,1.8868},
{-8.1475,5.0967,-1.1762,0.1239},
{48.2399,4.3094,-8.4751,1.5590},
{-12.5051,9.0312,-2.3379,0.2537},
{-156.8880,92.6738,-19.6075,1.6186},
{-157.9420,87.3469,-17.1995,1.3205},
{778.8581,-376.5213,60.7164,1.2620},
{43.3854,-21.3563,3.5510,0.0441},

   },



   .K_Poly_Coefficient_L =
   {

{-146.0942,78.8418,-17.4104,-0.0980},
{-7.7419,3.7325,-1.3242,-0.0118},
{-65.5165,31.5850,-5.0613,-0.1807},
{-39.4167,18.8614,-3.1689,-0.1688},
{-167.6425,99.9386,-21.5237,1.8868},
{-8.1475,5.0967,-1.1762,0.1239},
{48.2399,4.3094,-8.4751,1.5590},
{-12.5051,9.0312,-2.3379,0.2537},
{-156.8880,92.6738,-19.6075,1.6186},
{-157.9420,87.3469,-17.1995,1.3205},
{778.8581,-376.5213,60.7164,1.2620},
{43.3854,-21.3563,3.5510,0.0441},

   },

},

.bmi088_rot =
  {
    .rot_mat =
      {
        {+0, -1, +0},
        {+1, +0, +0},
        {+0, +0, +1},
      },
  },



};




/* clang-format off */
//TODO: write your param
/* clang-format on */

void robot_init() {
  System::Start<Robot::DmRobot, Robot::DmRobot::Param>(param);
}
