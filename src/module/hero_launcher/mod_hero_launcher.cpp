#include "mod_hero_launcher.hpp"
#include "bsp_time.h"
using namespace Module;

#define LAUNCHER_TRIG_SPEED_MAX (8191)

using namespace Module;

Launcher::Launcher(Param& param, float control_freq)
    : param_(param), ctrl_lock_(true) {
  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    this->trig_actuator_.at(i) =
        new Component::PosActuator(param.trig_actr.at(i), control_freq);

    this->trig_motor_.at(i) =
        new Device::RMMotor(this->param_.trig_motor.at(i),
                            ("Launcher_Trig" + std::to_string(i)).c_str());
  }

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    this->fric_actuator_.at(i) =
        new Component::SpeedActuator(param.fric_actr.at(i), control_freq);

    this->fric_motor_.at(i) =
        new Device::RMMotor(this->param_.fric_motor.at(i),
                            ("Launcher_Fric" + std::to_string(i)).c_str());
  }

  auto event_callback = [](LauncherEvent event, Launcher* launcher) {
    launcher->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) { /* 根据event设置模式 */
      case CHANGE_FIRE_MODE_RELAX:
        launcher->SetFireMode(static_cast<FireMode>(RELAX));
        break;
      case CHANGE_FIRE_MODE_SAFE:
        launcher->SetFireMode(static_cast<FireMode>(SAFE));
        break;
      case CHANGE_FIRE_MODE_LOADED:
        launcher->SetFireMode(static_cast<FireMode>(LOADED));
        break;
      case LAUNCHER_START_FIRE: /* 摩擦轮开启条件下，开火控制fire为ture */
        if (launcher->fire_ctrl_.fire_mode_ == LOADED) {
          launcher->fire_ctrl_.fire = true;
        }
        break;

      case CHANGE_TRIG_MODE_SINGLE:
        launcher->SetTrigMode(static_cast<TrigMode>(SINGLE));
        break;
      case CHANGE_TRIG_MODE_BURST:
        launcher->SetTrigMode(static_cast<TrigMode>(BURST));
        break;
      case CHANGE_TRIG_MODE_CONTINUED:
        launcher->SetTrigMode(static_cast<TrigMode>(CONTINUED));
        break;
      case CHANGE_TRIG_MODE_STOP:
        launcher->SetTrigMode(static_cast<TrigMode>(STOP));
        break;
      case CHANGE_TRIG_MODE:
        launcher->SetTrigMode(static_cast<TrigMode>(
            (launcher->fire_ctrl_.trig_mode_ + 1) % CONTINUED));
        break;
      default:
        break;
    }

    launcher->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Launcher*, LauncherEvent>(
      event_callback, this, this->param_.EVENT_MAP);



  auto launcher_thread = [](Launcher* launcher) {
    auto ref_sub = Message::Subscriber<Device::Referee::Data>("referee");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      ref_sub.DumpData(launcher->raw_ref_);

      launcher->PraseRef();

      launcher->ctrl_lock_.Wait(UINT32_MAX);

      launcher->UpdateFeedback();
      launcher->Control();

      launcher->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      launcher->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(launcher_thread, this, "launcher_thread",
                       MODULE_HERO_LAUNCHER_TASK_STACK_DEPTH,
                       System::Thread::MEDIUM);

}

void Launcher::UpdateFeedback()
{
  const float LAST_TRIG_MOTOR_ANGLE = this->trig_motor_[0]->GetAngle();

  for (size_t i =0; i < LAUNCHER_ACTR_FRIC_NUM; i++)
  {
   this->fric_motor_[i]->Update();
   speed_[i] = fric_motor_[i]->GetSpeed();
  }
  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++)
  {
   this->trig_motor_[i]->Update();
  }

  const float DELTA_MOTOR_ANGLE =
      this->trig_motor_[0]->GetAngle() - LAST_TRIG_MOTOR_ANGLE;
  this->trig_angle_ += DELTA_MOTOR_ANGLE / this->param_.trig_gear_ratio;
}


void Launcher::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  this->HeatLimit();

  /* 发弹量设置 */
  uint32_t max_burst = 0;

  switch (this->fire_ctrl_.trig_mode_)
  {
    case SINGLE: /* 点射开火模式 */
      max_burst = 1;
      break;
    case BURST: /* 爆发开火模式 */
      max_burst = 5;
      break;
    case STOP:
      max_burst = 0;
      break;
    case CONTINUED:
      max_burst = this->heat_ctrl_.available_shot;
    default:
      max_burst = 1;
      break;
  }
  /*执行发弹量*/
  switch (this->fire_ctrl_.trig_mode_) {
    case SINGLE: /* 点射开火模式 */
    case BURST:  /* 爆发开火模式 */
    case STOP:
    // case CONTINUED:

      /* 计算是否是第一次按下开火键 */
      this->fire_ctrl_.first_pressed_fire = this->fire_ctrl_.fire && !this->fire_ctrl_.last_fire;
      this->fire_ctrl_.last_fire = this->fire_ctrl_.fire;

      /* 设置要发射多少弹丸 */
      if (this->fire_ctrl_.first_pressed_fire && !this->fire_ctrl_.to_launch)
      {
       this->fire_ctrl_.to_launch =
         MIN(max_burst,(this->heat_ctrl_.available_shot - this->fire_ctrl_.launched));
      }

      /* 以下逻辑保证触发后一定会打完预设的弹丸，完成爆发 */
      if (this->fire_ctrl_.launched >= this->fire_ctrl_.to_launch) {
        this->fire_ctrl_.launch_delay = UINT32_MAX;
        this->fire_ctrl_.launched = 0;
        this->fire_ctrl_.to_launch = 0;
        this->fire_ctrl_.fire = false;
      } else {
        this->fire_ctrl_.launch_delay = this->param_.min_launch_delay;
      }
      break;

      case CONTINUED:
      /* 持续开火模式 */
      this->fire_ctrl_.launch_delay = UINT32_MAX;

      if (max_burst > 0)
      {
       this->fire_ctrl_.launch_delay = this->param_.min_launch_delay;
      }

      break;

    default:
      break;
  }

  /* 根据模式选择是否使用计算出来的值 */
  switch (this->fire_ctrl_.fire_mode_)
   {
    case RELAX:
    case SAFE:
      this->fire_ctrl_.bullet_speed = 0.0f;
      this->fire_ctrl_.launch_delay = UINT32_MAX;
      for (size_t i =0; i < LAUNCHER_ACTR_FRIC_NUM; i++)
      {this->setpoint_.fric_rpm_[i]=0.0f;}
    break;
    case LOADED:
      this->setpoint_.fric_rpm_[1] =  this->param_.fric_speed_1;
      this->setpoint_.fric_rpm_[0] = -this->setpoint_.fric_rpm_[1];
      this->setpoint_.fric_rpm_[2] =  this->param_.fric_speed_2;
      this->setpoint_.fric_rpm_[3] = -this->setpoint_.fric_rpm_[2];

      break;
  }



   /*一个拨弹周期到了之后 */
   //TODO英雄拨弹周期较长 防卡弹执行速度较慢 建议修改
   if ((bsp_time_get_ms() - this->fire_ctrl_.last_launch) >= this->fire_ctrl_.launch_delay)
   {
    /*如果拨弹盘没转到位 就认为是卡弹*/
    if ((fire_ctrl_.last_trig_angle - trig_angle_) / M_2PI * this->param_.num_trig_tooth < 0.8)
    {
     /*如果该拨弹盘允许反转*/
     if(this->param_.allow_reverse)
     {
      if(fire_ctrl_.stall)/*先复位*/
      {
       float tmp = this->setpoint_.trig_angle_;
       this->setpoint_.trig_angle_ = fire_ctrl_.last_trig_angle;
       fire_ctrl_.last_trig_angle = tmp;
       this->fire_ctrl_.last_launch = bsp_time_get_ms();
       fire_ctrl_.stall = false;
      }
      else/*再拨一次试试*/
      {
       fire_ctrl_.last_trig_angle = this->setpoint_.trig_angle_;
       this->setpoint_.trig_angle_ -= M_2PI / this->param_.num_trig_tooth;
       this->fire_ctrl_.last_launch = bsp_time_get_ms();
       fire_ctrl_.stall = true;
      }
     }
    }
    else/*正常拨弹*/
    {
      fire_ctrl_.last_trig_angle = this->setpoint_.trig_angle_;
      this->setpoint_.trig_angle_ -= M_2PI / this->param_.num_trig_tooth;
      this->fire_ctrl_.launched++;
      this->fire_ctrl_.last_launch = bsp_time_get_ms();
    }
   }




  /* 计算摩擦轮和拨弹盘并输出 */
  switch (this->fire_ctrl_.fire_mode_) {
    case RELAX:
      for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        this->trig_motor_[i]->Relax();
      }
      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        this->fric_motor_[i]->Relax();
      }

      break;

    case SAFE:
    case LOADED:
      for (int i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        /* 控制拨弹电机 */
        float trig_out = this->trig_actuator_[i]->Calculate(
            this->setpoint_.trig_angle_,
            this->trig_motor_[i]->GetSpeed() / LAUNCHER_TRIG_SPEED_MAX,
            this->trig_angle_, this->dt_);

        this->trig_motor_[i]->Control(trig_out);
      }

      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        /* 控制摩擦轮 */
        float fric_out = this->fric_actuator_[i]->Calculate(
            this->setpoint_.fric_rpm_[i], this->fric_motor_[i]->GetSpeed(),
            this->dt_);

        this->fric_motor_[i]->Control(fric_out);
      }
      break;
  }
}
/* 拨弹盘模式 */
/* SINGLE,BURST,CONTINUED,  */
void Launcher::SetTrigMode(TrigMode mode) {
  if (mode == this->fire_ctrl_.trig_mode_) {
    return;
  }

  this->fire_ctrl_.trig_mode_ = mode;
}
/* 设置摩擦轮模式 */
/* RELEX SAFE LOADED三种模式可以选择 */
void Launcher::SetFireMode(FireMode mode) {
  if (mode == this->fire_ctrl_.fire_mode_) { /* 未更改，return */
    return;
  }

  fire_ctrl_.fire = false;

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    this->fric_actuator_[i]->Reset();
  } /* reset 所有电机执行器PID等参数 */

  if (mode == LOADED) {
    this->fire_ctrl_.to_launch = 0;
  }

  this->fire_ctrl_.fire_mode_ = mode;
}

void Launcher::HeatLimit() {
  if (this->ref_.status == Device::Referee::RUNNING) {
    /* 根据机器人型号获得对应数据 */
    if (this->param_.model == LAUNCHER_MODEL_42MM) {
      this->heat_ctrl_.heat = this->ref_.power_heat.launcher_42_heat;
      this->heat_ctrl_.heat_limit = this->ref_.robot_status.shooter_heat_limit;
      this->heat_ctrl_.speed_limit = BULLET_SPEED_LIMIT_42MM;
      this->heat_ctrl_.cooling_rate =
          this->ref_.robot_status.shooter_cooling_value;
      this->heat_ctrl_.heat_increase = GAME_HEAT_INCREASE_42MM;
    } else if (this->param_.model == LAUNCHER_MODEL_17MM) {
      this->heat_ctrl_.heat = this->ref_.power_heat.launcher_id1_17_heat;
      this->heat_ctrl_.heat_limit = this->ref_.robot_status.shooter_heat_limit;
      this->heat_ctrl_.speed_limit = BULLET_SPEED_LIMIT_17MM;
      this->heat_ctrl_.cooling_rate =
          this->ref_.robot_status.shooter_cooling_value;
      this->heat_ctrl_.heat_increase = GAME_HEAT_INCREASE_17MM;
    }
    /* 检测热量更新后,计算可发射弹丸（裁判系统数据更新有延迟） */
    if ((this->heat_ctrl_.heat != this->heat_ctrl_.last_heat) ||
        this->heat_ctrl_.available_shot == 0 || (this->heat_ctrl_.heat == 0)) {
      this->heat_ctrl_.available_shot = static_cast<uint32_t>(
          floorf((this->heat_ctrl_.heat_limit - this->heat_ctrl_.heat) /
                 this->heat_ctrl_.heat_increase));
      this->heat_ctrl_.last_heat = this->heat_ctrl_.heat;
    }
    // this->fire_ctrl_.bullet_speed = this->heat_ctrl_.speed_limit;
  } else {
    /* 裁判系统离线，不启用热量控制 */
    this->heat_ctrl_.available_shot = 10;
    // this->fire_ctrl_.bullet_speed = this->param_.default_bullet_speed;
  }
}

void Launcher::PraseRef()
{
  memcpy(&(this->ref_.power_heat), &(this->raw_ref_.power_heat),
         sizeof(this->ref_.power_heat));
  memcpy(&(this->ref_.robot_status), &(this->raw_ref_.robot_status),
         sizeof(this->ref_.robot_status));
  memcpy(&(this->ref_.launcher_data), &(this->raw_ref_.launcher_data),
         sizeof(this->ref_.launcher_data));
  this->ref_.status = this->raw_ref_.status;
}
