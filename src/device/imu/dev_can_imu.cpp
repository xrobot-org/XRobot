#include "dev_can_imu.hpp"

using namespace Device;

IMU::IMU(IMU::Param &param)
    : param_(param),
      accl_((param.tp_name_prefix + std::string("_accl")).c_str(), true),
      gyro_((param.tp_name_prefix + std::string("_gyro")).c_str(), true),
      eulr_((param.tp_name_prefix + std::string("_eulr")).c_str(), true) {
  DECLARE_MESSAGE_FUN(rx_callback) {
    MESSAGE_GET_DATA(CAN::Pack, rx);
    MESSAGE_GET_ARG(IMU, imu);

    if (rx->index == imu->param_.index && rx->data[0] == IMU_DEVICE_ID) {
      imu->recv_.SendFromISR(rx);
    }

    MESSAGE_FUN_PASSED();
  };

  DECLARE_TOPIC(imu_tp, param.tp_name_prefix, false);
  imu_tp.RegisterCallback(rx_callback, this);

  CAN::Subscribe(imu_tp, this->param_.can, this->param_.index, 1);

  auto imu_thread = [](void *arg) {
    IMU *imu = static_cast<IMU *>(arg);

    while (1) {
      /* 读取裁判系统信息 */
      imu->Update();
      /* 一定时间长度内接收不到电容反馈值，使电容离线 */
      imu->Offline();

      /* 运行结束，等待下一次唤醒 */
      System::Thread::Sleep(1);
    }
  };

  THREAD_DECLEAR(this->thread_, imu_thread, 256, System::Thread::Medium, this);
}

void IMU::Update() {
  CAN::Pack rx;
  while (this->recv_.Receive(&rx, 0)) {
    this->Decode(rx);
    this->online_ = true;
    this->last_online_time_ = System::Thread::GetTick();
  }
}

bool IMU::Decode(CAN::Pack &rx) {
  int16_t *tmp = (int16_t *)rx.data;
  switch (rx.data[1]) {
    case ACCL_DATA_ID:
      this->accl_.data_.x = tmp[1] * 6.0f / (float)INT16_MAX;
      this->accl_.data_.y = tmp[2] * 6.0f / (float)INT16_MAX;
      this->accl_.data_.z = tmp[3] * 6.0f / (float)INT16_MAX;
      this->accl_.Publish();
      break;
    case GYRO_DATA_ID:
      this->gyro_.data_.x = tmp[1] * 20.0f / (float)INT16_MAX;
      this->gyro_.data_.y = tmp[2] * 20.0f / (float)INT16_MAX;
      this->gyro_.data_.z = tmp[3] * 20.0f / (float)INT16_MAX;
      this->gyro_.Publish();
      break;
    case EULR_DATA_ID:
      this->eulr_.data_.pit = tmp[1] * M_2PI / (float)INT16_MAX;
      this->eulr_.data_.rol = tmp[2] * M_2PI / (float)INT16_MAX;
      this->eulr_.data_.yaw = tmp[3] * M_2PI / (float)INT16_MAX;
      this->eulr_.Publish();
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
