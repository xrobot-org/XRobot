#include "dev_can_imu.hpp"

using namespace Device;

IMU::IMU(IMU::Param &param)
    : param_(param),
      accl_tp_((param.tp_name_prefix + std::string("_accl")).c_str()),
      gyro_tp_((param.tp_name_prefix + std::string("_gyro")).c_str()),
      eulr_tp_((param.tp_name_prefix + std::string("_eulr")).c_str()) {
  auto rx_callback = [](CAN::Pack &rx, IMU *imu) {
    if (rx.index == imu->param_.index && rx.data[0] == IMU_DEVICE_ID) {
      imu->recv_.SendFromISR(rx);
    }

    return true;
  };

  auto imu_tp = Message::Topic<CAN::Pack>(param.tp_name_prefix);
  imu_tp.RegisterCallback(rx_callback, this);

  CAN::Subscribe(imu_tp, this->param_.can, this->param_.index, 1);

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

  this->thread_.Create(imu_thread, this, "imu_thread", 256,
                       System::Thread::Medium);
}

void IMU::Update() {
  CAN::Pack rx;
  while (this->recv_.Receive(rx, 0)) {
    this->Decode(rx);
    this->online_ = true;
    this->last_online_time_ = System::Thread::GetTick();
  }
}

bool IMU::Decode(CAN::Pack &rx) {
  int16_t *tmp = (int16_t *)rx.data;
  switch (rx.data[1]) {
    case ACCL_DATA_ID:
      this->accl_.x = tmp[1] * 6.0f / (float)INT16_MAX;
      this->accl_.y = tmp[2] * 6.0f / (float)INT16_MAX;
      this->accl_.z = tmp[3] * 6.0f / (float)INT16_MAX;
      this->accl_tp_.Publish(this->accl_);
      break;
    case GYRO_DATA_ID:
      this->gyro_.x = tmp[1] * 20.0f / (float)INT16_MAX;
      this->gyro_.y = tmp[2] * 20.0f / (float)INT16_MAX;
      this->gyro_.z = tmp[3] * 20.0f / (float)INT16_MAX;
      this->gyro_tp_.Publish(this->gyro_);
      break;
    case EULR_DATA_ID:
      this->eulr_.pit = tmp[1] * M_2PI / (float)INT16_MAX;
      this->eulr_.rol = tmp[2] * M_2PI / (float)INT16_MAX;
      this->eulr_.yaw = tmp[3] * M_2PI / (float)INT16_MAX;
      this->eulr_tp_.Publish(this->eulr_);
      break;
    default:
      return false;
  }

  this->online_ = 1;
  this->last_online_time_ = System::Thread::GetTick();

  return true;
}

bool IMU::Offline() {
  if (System::Thread::GetTick() - this->last_online_time_ > 10) {
    this->online_ = 0;
  }

  return true;
}
