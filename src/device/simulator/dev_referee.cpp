/*
  裁判系统抽象。
*/

#include "dev_referee.hpp"

using namespace Device;

Referee::Referee() {
  auto ref_recv_thread = [](Referee *ref) {
    while (1) {
      ref->Prase();

      /* 发布裁判系统数据 */
      ref->ref_data_tp_.Publish(ref->ref_data_);

      ref->recv_thread_.Sleep(10);
    }
  };

  this->recv_thread_.Create(ref_recv_thread, this, "ref_recv_thread", 256,
                            System::Thread::REALTIME);
}

void Referee::Prase() {
  this->ref_data_.status = RUNNING;
  this->ref_data_.power_heat.launcher_id1_17_heat = REF_HEAT_LIMIT_17;
  this->ref_data_.power_heat.launcher_42_heat = REF_HEAT_LIMIT_42;
  this->ref_data_.robot_status.launcher_id1_17_speed_limit = REF_LAUNCH_SPEED;
  this->ref_data_.robot_status.launcher_42_speed_limit = REF_LAUNCH_SPEED;
  this->ref_data_.robot_status.chassis_power_limit = REF_POWER_LIMIT;
  this->ref_data_.power_heat.chassis_pwr_buff = REF_POWER_BUFF;
}
