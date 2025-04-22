#include "mod_mit_control.hpp"

using namespace Module;

MitControl::MitControl(Param& param)
    :mitmotor_1_(param.mitmotor_1,"mitmotor_1"),
     mitmotor_2_(param.mitmotor_2,"mitmotor_2"),
     control_lock_(true)
{
  auto ctrl_thread=[](MitControl* mitcontrol){

    uint32_t last_online_time = bsp_time_get_ms();

    while(1){
      mitcontrol->out_ = 0.02;

      mitcontrol->UpdateFeedback();

      mitcontrol->control();

      mitcontrol->thread_.SleepUntil(2,last_online_time);
    }
  };
  this->mitmotor_1_.Enable();
  this->mitmotor_2_.Enable();

  this->thread_.Create(ctrl_thread,this,"mitcontrol",1024,System::Thread::MEDIUM);
}

void MitControl::UpdateFeedback(){this->mitmotor_1_.Update();this->mitmotor_2_.Update();}

void MitControl::control(){
  // this->now_ = bsp_time_get();
  // this->dt_ = this->now_-last_wakeup_;
  // this->last_wakeup_=this->now_;

  this->mitmotor_1_.SetMit(out_);
  this->mitmotor_2_.SetMit(out_);
}