#include "dev_can_imu.hpp"

#include "bsp_time.h"

using namespace Device;

IMU::IMU(IMU::Param &param)
    : param_(param),
      accl_tp_((param.tp_name_prefix + std::string("_accl")).c_str()),
      gyro_tp_((param.tp_name_prefix + std::string("_gyro")).c_str()),
      eulr_tp_((param.tp_name_prefix + std::string("_eulr")).c_str()),
      quat_tp_((param.tp_name_prefix + std::string("_quat")).c_str()) {
  auto rx_callback = [](Can::Pack &rx, IMU *imu) {
    if (rx.index == imu->param_.index && rx.data[0] == 0) {
    }
    return true;
  };

  auto imu_tp = Message::Topic<Can::Pack>(param.tp_name_prefix);
  imu_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(imu_tp, this->param_.can, this->param_.index, 5);

  auto imu_thread = [](IMU *imu) {
    while (1) {
      imu->Update();

      imu->Offline();

      /* 运行结束，等待下一次唤醒 */
      System::Thread::Sleep(1);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread",
                       DEVICE_CAN_IMU_TASK_STACK_DEPTH,
                       System::Thread::REALTIME);
}

void IMU::Update() {
  Can::Pack rx;
  while (this->recv_.Receive(rx)) {
    this->Decode(rx);
    this->online_ = true;
    this->last_online_time_ = bsp_time_get_ms();
  }
}

bool IMU::Decode(Can::Pack &rx) {
  int16_t *tmp = reinterpret_cast<int16_t *>(rx.data);

  uint32_t offset = rx.index - param_.index;

  switch (offset) {
    case 0:
      this->accl_.x = static_cast<float>(tmp[0]) * 16.0f / INT16_MAX;
      this->accl_.y = static_cast<float>(tmp[1]) * 16.0f / INT16_MAX;
      this->accl_.z = static_cast<float>(tmp[2]) * 16.0f / INT16_MAX;
      this->accl_tp_.Publish(this->accl_);
      break;
    case 1:
      this->gyro_.x = static_cast<float>(tmp[0]) * 34.90658502f / INT16_MAX;
      this->gyro_.y = static_cast<float>(tmp[1]) * 34.90658502f / INT16_MAX;
      this->gyro_.z = static_cast<float>(tmp[2]) * 34.90658502f / INT16_MAX;
      this->gyro_tp_.Publish(this->gyro_);
      break;
    case 3:
      this->eulr_.pit = static_cast<float>(tmp[0]) * M_2PI / INT16_MAX;
      this->eulr_.rol = static_cast<float>(tmp[1]) * M_2PI / INT16_MAX;
      this->eulr_.yaw = static_cast<float>(tmp[2]) * M_2PI / INT16_MAX;
      this->eulr_tp_.Publish(this->eulr_);
      break;
    case 4:
      this->quat_.q0 = static_cast<float>(tmp[0]) * 2.0f / INT16_MAX;
      this->quat_.q1 = static_cast<float>(tmp[1]) * 2.0f / INT16_MAX;
      this->quat_.q2 = static_cast<float>(tmp[2]) * 2.0f / INT16_MAX;
      this->quat_.q3 = static_cast<float>(tmp[3]) * 2.0f / INT16_MAX;
      this->quat_tp_.Publish(this->quat_);
      break;
    default:
      return false;
  }
  return true;
}

bool IMU::Offline() {
  if (bsp_time_get_ms() - this->last_online_time_ > 100) {
    this->online_ = 0;
  }

  return true;
}
