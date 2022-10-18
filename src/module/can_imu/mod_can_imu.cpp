#include "mod_can_imu.hpp"

#include "dev_can.hpp"

using namespace Module;

static Device::CAN::Pack send_buff;

CanIMU::CanIMU() {
  auto imu_thread = [](CanIMU *imu) {
    auto eulr_sub = Message::Subscriber("imu_eulr", imu->eulr_);
    auto gyro_sub = Message::Subscriber("imu_eulr", imu->eulr_);
    auto accl_sub = Message::Subscriber("imu_eulr", imu->eulr_);

    while (1) {
#if IMU_SEND_ACCL
      accl_sub.DumpData();
      imu->SendAccl();
#endif

#if IMU_SEND_GYRO
      gyro_sub.DumpData();
      imu->SendGyro();
#endif

#if IMU_SEND_EULR
      eulr_sub.DumpData();
      imu->SendEulr();
#endif

      imu->thread_.Sleep(1000 / IMU_SEND_FREQ);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread", 256,
                       System::Thread::Medium);
}

void CanIMU::SendAccl() {
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->accl_.x / 6.0f * (float)INT16_MAX;
  tmp[2] = this->accl_.y / 6.0f * (float)INT16_MAX;
  tmp[3] = this->accl_.z / 6.0f * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::ACCL_DATA_ID;
  send_buff.index = IMU_SEND_CAN_ID;
  Device::CAN::SendPack(BSP_CAN_1, send_buff);
}

void CanIMU::SendGyro() {
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->gyro_.x / 20.0f * (float)INT16_MAX;
  tmp[2] = this->gyro_.y / 20.0f * (float)INT16_MAX;
  tmp[3] = this->gyro_.z / 20.0f * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::GYRO_DATA_ID;
  Device::CAN::SendPack(BSP_CAN_1, send_buff);
}

void CanIMU::SendEulr() {
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->eulr_.pit / M_2PI * (float)INT16_MAX;
  tmp[2] = this->eulr_.rol / M_2PI * (float)INT16_MAX;
  tmp[3] = this->eulr_.yaw / M_2PI * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::EULR_DATA_ID;
  Device::CAN::SendPack(BSP_CAN_1, send_buff);
}
