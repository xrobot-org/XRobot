#include "dev_can_imu.hpp"

#include "bsp_time.h"

using namespace Device;

IMU::IMU(IMU::Param &param)
    : param_(param),
      accl_tp_((param.tp_name_prefix + std::string("_accl")).c_str()),
      gyro_tp_((param.tp_name_prefix + std::string("_gyro")).c_str()),
      eulr_tp_((param.tp_name_prefix + std::string("_eulr")).c_str()) {
  auto rx_callback = [](Can::Pack &rx, IMU *imu) {
    if (rx.index == imu->param_.index && rx.data[0] == IMU_DEVICE_ID) {
      // imu->recv_.SendFromISR(rx);
    }

    return true;
  };

  auto imu_tp = Message::Topic<Can::Pack>(param.tp_name_prefix);
  imu_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(imu_tp, this->param_.can, this->param_.index, 1);

  auto imu_thread = [](IMU *imu) {
    while (1) {
      /* 读取裁判系统信息 */
      imu->Update();
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
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
  switch (rx.data[1]) {
    case ACCL_DATA_ID:
      this->accl_.x = static_cast<float>(tmp[1]) * 6.0f / INT16_MAX;
      this->accl_.y = static_cast<float>(tmp[2]) * 6.0f / INT16_MAX;
      this->accl_.z = static_cast<float>(tmp[3]) * 6.0f / INT16_MAX;
      this->accl_tp_.Publish(this->accl_);
      break;
    case GYRO_DATA_ID:
      this->gyro_.x = static_cast<float>(tmp[1]) * 20.0f / INT16_MAX;
      this->gyro_.y = static_cast<float>(tmp[2]) * 20.0f / INT16_MAX;
      this->gyro_.z = static_cast<float>(tmp[3]) * 20.0f / INT16_MAX;
      this->gyro_tp_.Publish(this->gyro_);
      break;
    case EULR_DATA_ID:
      this->eulr_.pit = static_cast<float>(tmp[1]) * M_2PI / INT16_MAX;
      this->eulr_.rol = static_cast<float>(tmp[2]) * M_2PI / INT16_MAX;
      this->eulr_.yaw = static_cast<float>(tmp[3]) * M_2PI / INT16_MAX;
      this->eulr_tp_.Publish(this->eulr_);
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
