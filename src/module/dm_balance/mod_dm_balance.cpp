#include "mod_dm_balance.hpp"
#include "comp_vmc.hpp"
#include <tuple>

#define PI_2 1.571f

using namespace Module;

Balance::Balance(Param& param):
    param_(param),
    mode_(Balance::RELAX),
    ctrl_lock_(true)
{
    memset(&(this->cmd_),0,sizeof(this->cmd_));

    this->hip_motor_.at(0) = new Device::MitMotor(param.hip_motor_param.at(0),"hip_left_back");
    this->hip_motor_.at(1) = new Device::MitMotor(param.hip_motor_param.at(1),"hip_left_front");
    this->hip_motor_.at(2) = new Device::MitMotor(param.hip_motor_param.at(2),"hip_right_back");
    this->hip_motor_.at(3) = new Device::MitMotor(param.hip_motor_param.at(3),"hip_left_front");

    // this->wheel_motor_.at(0) = new Device::MitMotor(param.wheel_motor_param.at(0),"wheel_right");
    // this->wheel_motor_.at(1) = new Device::MitMotor(param.wheel_motor_param.at(1),"wheel_left");
     this->wheel_motor_.at(0) = new Device::RMMotor(param.wheel_motor_param.at(0),"wheel_left");
     this->wheel_motor_.at(1) = new Device::RMMotor(param.wheel_motor_param.at(1),"wheel_right");

    this->leglength_pid_.at(0) = new Component::PID(param.leglength_pid_param.at(0),333.3f);
    this->leglength_pid_.at(1) = new Component::PID(param.leglength_pid_param.at(1),333.3f);

    this->Tp_pid_ = new Component::PID(param.Tp_pid_param,333.3f);
    this->roll_pid = new Component::PID(param.roll_pid_param,333.3f);
    this->yaw_pid_ = new Component::PID(param.yaw_pid_param,333.3f);

    this->leg_.at(0) = new Component::VMC(param_.leg_param.at(0),333.3f);
    this->leg_.at(1) = new Component::VMC(param_.leg_param.at(1),333.3f);


    ctrl_lock_.Post();




    auto event_callback = [](Balance::ChassisEvent event , Balance * chassis)
    {
     chassis->ctrl_lock_.Wait(UINT32_MAX);
     switch(event)
     {
       case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;

       case SET_MODE_DATA:
        chassis->SetMode(DATA);
        break;

       case SET_MODE_STAND:
        chassis->SetMode(STAND);
        break;

       default:
        break;
    }
    chassis->ctrl_lock_.Post();
    };

    Component::CMD::RegisterEvent<Balance*,ChassisEvent>
        (event_callback,this,this->param_.EVENT_MAP);

    auto chassis_thread = [](Balance* chassis)
    {

     auto cmd_sub = Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");

     auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl_abs");

     auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");

     auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");




     uint32_t last_online_time = bsp_time_get_ms();

     while(1)
     {
      eulr_sub.DumpData(chassis->eulr_); /* imu */
      gyro_sub.DumpData(chassis->gyro_); /* imu */


      cmd_sub.DumpData(chassis->cmd_);
      accl_sub.DumpData(chassis->accl_);
      chassis->ctrl_lock_.Wait(UINT32_MAX);
      chassis->MotorSetAble();
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      chassis->thread_.SleepUntil(3,last_online_time);


     }
    };

    this->thread_.Create(chassis_thread,this,"chassis_thread",4096 , System::Thread::MEDIUM);

}



void Balance::MotorSetAble()
{




  if (this->hip_motor_flag_ == 0)
  {
    this->hip_motor_[0]->Relax();
    this->hip_motor_[1]->Relax();
    this->hip_motor_[2]->Relax();
    this->hip_motor_[3]->Relax();
  this->dm_motor_flag_ = 0;
  }

  else
  {
    if (this->dm_motor_flag_ == 0)
    {

    for (int i=0;i<5;i++)
    {
    this->hip_motor_[0]->Enable();
    }
    for (int i=0;i<5;i++)
    {
    this->hip_motor_[1]->Enable();
    }
        for (int i=0;i<5;i++)
    {
    this->hip_motor_[2]->Enable();
    }
    for (int i=0;i<5;i++)
    {
    this->hip_motor_[3]->Enable();
    }

    this->dm_motor_flag_ = 1;



    }
//    this->hip_motor_[2]->Enable();
//    this->hip_motor_[3]->Enable();
  };

}


void Balance::UpdateFeedback()
{
    // this->hip_motor_[1]->Update();
    // this->hip_motor_[0]->Update();

    // this->hip_motor_[2]->Update();
    // this->hip_motor_[3]->Update();

    this->wheel_motor_[0]->Update();
    this->wheel_motor_[1]->Update();

    this->leg_argu_[0].phi4_= - this->hip_motor_[1]->GetAngle() - this->param_.mech_zero[1] ;                  //前关节角度
    this->leg_argu_[0].phi1_= - this->hip_motor_[0]->GetAngle() - this->param_.mech_zero[0] + 3.1416f ;        //后关节角度

    this->leg_argu_[1].phi4_= - (-this->hip_motor_[3]->GetAngle() + this->param_.mech_zero[3]) - 3.1416  ;     //前关节角度
    this->leg_argu_[1].phi1_= - (-this->hip_motor_[2]->GetAngle() + this->param_.mech_zero[2]) ;               //后关节角度

    // this->leg_argu_[0].phi4_=   - 0.45728f;  //前髋关节
    // this->leg_argu_[0].phi1_=  3.59887f;//后髋关节
    // this->leg_argu_[1].phi4_=   - 0.45728f;
    // this->leg_argu_[1].phi1_=   3.59887;


    // this->leg_argu_[0].phi1_=   0;
    // this->leg_argu_[0].phi4_=   M_PI;
    // this->leg_argu_[1].phi1_=   0;
    // this->leg_argu_[1].phi4_=   M_PI;




    // phi1_[0] = this->hip_motor_[0]->GetAngle() - this->param_.mech_zero[0] + PI_2;//+2_pi
    // phi1_[1] = this->hip_motor_[2]->GetAngle() - this->param_.mech_zero[1] + PI_2;
    // phi4_[0] = this->hip_motor_[1]->GetAngle() - this->param_.mech_zero[2] + PI_2;
    // phi4_[1] = this->hip_motor_[3]->GetAngle() - this->param_.mech_zero[3] + PI_2;





}


void Balance::Control()
{

  this->now_ = bsp_time_get();
  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;
 // this->real_dt_ = dt_ / 1000000;     /*建模时以s为单位*/

  this->pit_ = eulr_.pit;
  if (this->eulr_.pit > 3.141f)
  {
    this->pit_ =  eulr_.pit - 6.283185f ;
  }



  switch(this->mode_)
  {
    case Balance::RELAX:

     this->move_vec_.vx = 0.0f;
     this->move_vec_.vy = 0.0f;
     this->move_vec_.wz = 0.0f;
    break;

    case Balance::DATA:
    case Balance::STAND:

     this->move_vec_.vx = 0.0f;
     this->move_vec_.vy =  this->cmd_.y;
     this->shift_x_ = this->shift_x_ + move_vec_.vy *  this->dt_ / 2000000;  //1000.0f?
     this->move_vec_.wz = this->cmd_.z;

    break;

    default:
      XB_ASSERT(false);
      return;
  }

  switch(this->mode_)
  {
    case Balance::RELAX:


      this->hip_motor_flag_ = 0;
    break;

    case Balance::DATA:
    case Balance::STAND:

        std::tuple<float,float,float>result0 = this->leg_[0]->VMCsolve
            (leg_argu_[0].phi1_, leg_argu_[0].phi4_,  this->pit_,  -this->gyro_.x, this->dt_ / 1000000);
        this->leg_argu_[0].L0 = std::get<0>(result0);
        this->leg_argu_[0].theta = -std::get<1>(result0);
        this->leg_argu_[0].d_theta = -std::get<2>(result0);

        std::tuple<float,float,float>result1 = this->leg_[1]->VMCsolve
            (leg_argu_[1].phi1_, leg_argu_[1].phi4_,  this->pit_,  -this->gyro_.x, this->dt_ / 1000000);
        this->leg_argu_[1].L0 = std::get<0>(result1);
        this->leg_argu_[1].theta = -std::get<1>(result1) ;
        this->leg_argu_[1].d_theta = -std::get<2>(result1);


        // this->body_argu_.d_x = this->body_argu_.d_x + this->accl_.y * dt_/1000000.0f;
      this->body_argu_.d_x = (-wheel_motor_[0]->GetSpeed() + wheel_motor_[1]->GetSpeed())*(0.22f)/120.f;

      this->body_argu_.x = this->body_argu_.x + this->body_argu_.d_x *  this->dt_ / 1000000; //1000.0f;?
        //  this->body_argu_.d_x = this->accl_.y * dt_/1000000.0f +this->body_argu_.d_x; //尝试使用陀螺仪
        //  this->body_argu_.x =    this->body_argu_.x  + this->body_argu_.d_x * dt_/1000000.0f;

        for(int i=0;i<12;i++)
	    {
		    leg_argu_[0].LQR_K[i] = this->leg_[0]->LQR_K_calc(&this->param_.K_Poly_Coefficient_L[i][0],leg_argu_[0].L0 );
	    }


        for(int i=0;i<12;i++)
	    {
		    leg_argu_[1].LQR_K[i] = this->leg_[1]->LQR_K_calc(&this->param_.K_Poly_Coefficient_R[i][0],leg_argu_[1].L0 );
	    }

      //   for(int i=0;i<12;i++)
	    // {
		  //   leg_argu_[0].LQR_K[i] = this->param_.LQR_K_length[i] ;
	    // }

      //   for(int i=0;i<12;i++)
	    // {
		  //   leg_argu_[1].LQR_K[i]= this->param_.LQR_K_length[i];
      // }

        leg_argu_[0].Tw =
            (
                leg_argu_[0].LQR_K[0]*(-leg_argu_[0].theta+0.0f)+
                leg_argu_[0].LQR_K[1]*(-leg_argu_[0].d_theta+0.0f)+
                leg_argu_[0].LQR_K[2]*(-body_argu_.x  + this->shift_x_)+
                leg_argu_[0].LQR_K[3]*(-body_argu_.d_x + 0.0f)+
                leg_argu_[0].LQR_K[4]*(-this->pit_ + 0.0f)+
                leg_argu_[0].LQR_K[5]*(-this->gyro_.x + 0.0f)
            );
        leg_argu_[0].Tp =
            (
                leg_argu_[0].LQR_K[6]*(-leg_argu_[0].theta + 0.0f)+
                leg_argu_[0].LQR_K[7]*(-leg_argu_[0].d_theta + 0.0f)+
                leg_argu_[0].LQR_K[8]*(-body_argu_.x + this->shift_x_)+
                leg_argu_[0].LQR_K[9]*(-body_argu_.d_x + 0.0f)+
                leg_argu_[0].LQR_K[10]*(-this->pit_ + 0.0f)+
                leg_argu_[0].LQR_K[11]*(-this->gyro_.x + 0.0f)
            );
        leg_argu_[1].Tw =
            (
                leg_argu_[1].LQR_K[0]*(-leg_argu_[1].theta + 0.0f)+
                leg_argu_[1].LQR_K[1]*(-leg_argu_[1].d_theta + 0.0f)+
                leg_argu_[1].LQR_K[2]*(-body_argu_.x + this->shift_x_)+
                leg_argu_[1].LQR_K[3]*(-body_argu_.d_x + 0.0f)+
                leg_argu_[1].LQR_K[4]*(-this->pit_ + 0.0f)+
                leg_argu_[1].LQR_K[5]*(-this->gyro_.x + 0.0f)
            );
        leg_argu_[1].Tp =
            (
                leg_argu_[1].LQR_K[6]*(-leg_argu_[1].theta + 0.0f)+
                leg_argu_[1].LQR_K[7]*(-leg_argu_[1].d_theta + 0.0f)+
                leg_argu_[1].LQR_K[8]*(-body_argu_.x + this->shift_x_)+
                leg_argu_[1].LQR_K[9]*(-body_argu_.d_x +0.0f)+
                leg_argu_[1].LQR_K[10]*(-this->pit_ + 0.0f)+
                leg_argu_[1].LQR_K[11]*(-this->gyro_.x + 0.0f)
            );






   this->leg_argu_[0].Delat_L0 =  this->param_.target_L0[0] + this->cmd_.z/20.0f;
   this->leg_argu_[1].Delat_L0 =  this->param_.target_L0[1] + this->cmd_.z/20.0f;





  leg_argu_[0].F0 = 4.0f/cosf(leg_argu_[0].theta) + leglength_pid_.at(0)->Calculate(this->leg_argu_[0].Delat_L0,this->leg_argu_[0].L0,this->dt_);
  leg_argu_[1].F0 = 4.0f/cosf(leg_argu_[1].theta) + leglength_pid_.at(1)->Calculate(this->leg_argu_[1].Delat_L0,this->leg_argu_[1].L0,this->dt_);
        // leg_argu_[1].F0 = + leglength_pid_[1]->Calculate(this->param_.target_L0[1],this->leg_argu_[1].L0,dt_);

  this->leg_argu_[0].Delta_Tp = -Tp_pid_->Calculate(this->leg_argu_[0].theta,this->leg_argu_[1].theta,this->dt_);
  this->leg_argu_[1].Delta_Tp = - this->leg_argu_[0].Delta_Tp;

        std::tuple<float,float>result3 = leg_[0]->VMCinserve
            (leg_argu_[0].phi1_,leg_argu_[0].phi4_,leg_argu_[0].Tp + this->leg_argu_[0].Delta_Tp, leg_argu_[0].F0);
        this->leg_argu_[0].T1 = std::get<0>(result3);
        this->leg_argu_[0].T2 = std::get<1>(result3);


        std::tuple<float,float>result4 = leg_[1]->VMCinserve
            (leg_argu_[1].phi1_,leg_argu_[1].phi4_,leg_argu_[1].Tp + this->leg_argu_[1].Delta_Tp, leg_argu_[1].F0);
        this->leg_argu_[1].T1 = std::get<0>(result4);
        this->leg_argu_[1].T2 = std::get<1>(result4);


        this->body_argu_.target_yaw = this->body_argu_.target_yaw  -  this->cmd_.x /  1000.0f;

        this->wheel_motor_out_[0] = this->leg_argu_[0].Tw*3.2f - this->yaw_pid_->Calculate(this->body_argu_.target_yaw, this->eulr_.yaw ,dt_);

        this->wheel_motor_out_[1] = this->leg_argu_[1].Tw*3.2f + this->yaw_pid_->Calculate(this->body_argu_.target_yaw, this->eulr_.yaw ,dt_);

    this->hip_motor_out_[0] = this->leg_argu_[0].T1;
    this->hip_motor_out_[1] = this->leg_argu_[0].T2;
    this->hip_motor_out_[2] = this->leg_argu_[1].T1;
    this->hip_motor_out_[3] = this->leg_argu_[1].T2;


        this->hip_motor_flag_ = 1;


       break;

  }

 /*安全限幅*/


    clampf(&wheel_motor_out_[0],-0.75f,0.75f);
    clampf(&wheel_motor_out_[1],-0.75f,0.75f);




    clampf(&hip_motor_out_[0],-3.0f,3.0f);
    clampf(&hip_motor_out_[1],-3.0f,3.0f);
    clampf(&hip_motor_out_[2],-3.0f,3.0f);
    clampf(&hip_motor_out_[3],-3.0f,3.0f);

   if(fabs(wheel_motor_[0]->GetSpeed())>1000 || fabs(wheel_motor_[1]->GetSpeed())>1000)
   {
    wheel_motor_out_[0] = 0;
    wheel_motor_out_[1] = 0;
   }


        if (fabs(this->pit_ ) > 0.5f)
        {

         this->hip_motor_flag_ = 0;
        }
        // this->hip_motor_flag_ = 1;





    switch(this->mode_)
    {
     case Balance::RELAX:
        break;

    case Balance::DATA:



     this->wheel_motor_[0]->Relax();
     this->wheel_motor_[1]->Relax();

     hip_motor_[0]->SetMit(0.0f);
     hip_motor_[1]->SetMit(0.0f);
     hip_motor_[2]->SetMit(0.0f);
     hip_motor_[3]->SetMit(0.0f);
     break;


     case Balance::STAND:


        this->wheel_motor_[0]->Control(-wheel_motor_out_[0]);
        this->wheel_motor_[1]->Control(wheel_motor_out_[1]);


        hip_motor_[0]->SetMit(-this->hip_motor_out_[0]);
        hip_motor_[1]->SetMit(-this->hip_motor_out_[1]);
  
        hip_motor_[2]->SetMit(this->hip_motor_out_[2]);
        hip_motor_[3]->SetMit(this->hip_motor_out_[3]);

      break;

    }


}


void Balance::SetMode(Balance::Mode mode)
{
  if(mode == this->mode_)
  {
    return;
  }

  this->leg_[0]->Reset();
  this->leg_[1]->Reset();
  this->shift_x_ = 0.0f;
  this->body_argu_.d_x = 0.0f ;
  this->body_argu_.x = 0.0f;
  this->body_argu_.target_yaw = this->eulr_.yaw;


  this->mode_ = mode;

}
