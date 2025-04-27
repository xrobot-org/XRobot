#pragma once
#include "module.hpp"

#include "comp_vmc.hpp"
#include "comp_cmd.hpp"
#include "comp_pid.hpp"
#include "comp_cmd.hpp"
#include "dev_motor.hpp"
#include "dev_mit_motor.hpp"
#include "dev_rm_motor.hpp"
namespace Module
{
class Balance
{
 public:
  typedef enum
  {
    RELAX,
    DATA,
    STAND,
  }Mode;

  typedef enum
  {
    SET_MODE_RELAX,
    SET_MODE_DATA,
    SET_MODE_STAND,



 }ChassisEvent;



  typedef struct Param
  {
    std::array<Component::VMC::Param,2> leg_param{};
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
    std::array<Component::Type::CycleValue, 4> mech_zero;
    std::array<Component::PID::Param,2> leglength_pid_param{};
    Component::PID::Param Tp_pid_param{};
    Component::PID::Param roll_pid_param{};
    Component::PID::Param yaw_pid_param{};
    //Component::PID::Param leglength_pid_param{};

    std::array<Device::MitMotor::Param, 4> hip_motor_param;
    // std::array<Device::MitMotor::Param, 2> wheel_motor_param;
    std::array<Device::RMMotor::Param, 2> wheel_motor_param;
    // float wheel_radius;

    float target_L0[2];
    float target_F[2];

    float K_Poly_Coefficient_R[12][4];

    float K_Poly_Coefficient_L[12][4];


  } Param;

  Balance(Param &param);

  void MotorSetAble();

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

 private:

  Param param_;

  float dt_;
 // float real_dt_;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  float pit_;

  Mode mode_ ;

  //bool wheel_motor_flag_ = 1;
   bool hip_motor_flag_ = 1;
   bool dm_motor_flag_ = 1;

  std::array<Component::VMC*,2> leg_;
  std::array<Device::MitMotor*, 4> hip_motor_;
  // std::array<Device::MitMotor*, 2> wheel_motor_;
   std::array<Device::RMMotor*, 2> wheel_motor_;

  std::array<float, 4 >hip_motor_out_;
  std::array<float, 2 >wheel_motor_out_;

  // std::array<float, 2>phi1_;
  // std::array<float, 2>phi4_;

  struct
  {
    float phi1_;
    float phi4_;
    float L0;

    float theta;
    float d_theta;

    float Tw;
    float F0;
    float Tp;
    float T1;
    float T2;

    float Delat_L0;
    float Delta_Tp;
    float Delta_F;

    float LQR_K[12];

  }leg_argu_[2]; /*0右腿 1左腿*/

struct
{
    float x = 0;
    float d_x = 0 ;
    Component::Type::CycleValue target_yaw ;
    Component::Type::CycleValue target_roll ;


}body_argu_;



  Component::Type::MoveVector move_vec_;


  float shift_x_;



  std::array<Component::PID*,2>leglength_pid_;
  Component::PID* Tp_pid_;
  Component::PID* roll_pid;
  Component::PID* yaw_pid_;

  //Component::PID* leglength_pid_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;
  Component::CMD::ChassisCMD cmd_;
  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;
  Component::Type::Vector3 accl_;

};
}  // namespace Module
