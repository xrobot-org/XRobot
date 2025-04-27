#include "dev_laser.hpp"

#include "bsp_pwm.h"

using namespace Device;

Laser::Laser(bool auto_start){
   bsp_pwm_start(BSP_PWM_LASER);

  auto laser_thread = [](Laser* laser)
  {
    uint32_t last_online_time = bsp_time_get_ms();

    while(1)
    {
      laser->Set(1.0f);

      laser->thread_.SleepUntil(250,  last_online_time);

    };

  };
    if(auto_start)
    {
    this->thread_.Create(laser_thread,this,"laser_thread", DEVICE_LASER_TASK_STACK_DEPTH ,System::Thread::LOW);
    }
}

void Laser::Start() { bsp_pwm_start(BSP_PWM_LASER); }

bool Laser::Set(float duty_cycle) {
  if (duty_cycle > 1.001f) {
    return false;
  }
  bsp_pwm_set_comp(BSP_PWM_LASER, duty_cycle);

  return true;
}

void Laser::Stop() { bsp_pwm_stop(BSP_PWM_LASER); }
