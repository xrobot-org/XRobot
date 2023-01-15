#include "mod_can_imu.hpp"

#include "dev_can.hpp"

using namespace Module;

static Device::Can::Pack send_buff;

#if IMU_USE_IN_WEARLAB
static Device::WearLab::CanHeader header;
#endif

CanIMU::CanIMU() {
  auto imu_thread = [](CanIMU *imu) {
#if IMU_SEND_EULR
    auto eulr_sub = Message::Subscriber("imu_eulr", imu->eulr_);
#endif
#if IMU_SEND_QUAT
    auto quar_sub = Message::Subscriber("imu_quat", imu->quat_);
#endif
#if IMU_SEND_GYRO
    auto gyro_sub = Message::Subscriber("imu_gyro", imu->gyro_);
#endif
#if IMU_SEND_ACCL
    auto accl_sub = Message::Subscriber("imu_accl", imu->accl_);
#endif

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

#if IMU_SEND_QUAT
      quar_sub.DumpData();
      imu->SendQuat();
#endif

      imu->thread_.SleepUntil(1000 / IMU_SEND_FREQ);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread", 256,
                       System::Thread::Medium);
}

void CanIMU::SendAccl() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = IMU_SEND_CAN_ID;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::ACCL_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = fabsf(this->accl_.x) * 100000.0f;
  tmp->data1_symbol = this->accl_.x > 0 ? 0 : 1;
  tmp->data2 = fabsf(this->accl_.y) * 100000.0f;
  tmp->data2_symbol = this->accl_.y > 0 ? 0 : 1;
  tmp->data3 = fabsf(this->accl_.z) * 100000.0f;
  tmp->data3_symbol = this->accl_.z > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->accl_.x / 6.0f * (float)INT16_MAX;
  tmp[2] = this->accl_.y / 6.0f * (float)INT16_MAX;
  tmp[3] = this->accl_.z / 6.0f * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::ACCL_DATA_ID;
  send_buff.index = IMU_SEND_CAN_ID;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendGyro() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = IMU_SEND_CAN_ID;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::GYRO_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = fabsf(this->gyro_.x) * 20000.0f;
  tmp->data1_symbol = this->gyro_.x > 0 ? 0 : 1;
  tmp->data2 = fabsf(this->gyro_.y) * 20000.0f;
  tmp->data2_symbol = this->gyro_.y > 0 ? 0 : 1;
  tmp->data3 = fabsf(this->gyro_.z) * 20000.0f;
  tmp->data3_symbol = this->gyro_.z > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->gyro_.x / 20.0f * (float)INT16_MAX;
  tmp[2] = this->gyro_.y / 20.0f * (float)INT16_MAX;
  tmp[3] = this->gyro_.z / 20.0f * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::GYRO_DATA_ID;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendEulr() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = IMU_SEND_CAN_ID;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::EULR_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = fabsf(this->eulr_.pit) * 300000.0f;
  tmp->data1_symbol = this->eulr_.pit > 0 ? 0 : 1;
  tmp->data2 = fabsf(this->eulr_.rol) * 300000.0f;
  tmp->data2_symbol = this->eulr_.rol > 0 ? 0 : 1;
  tmp->data3 = fabsf(this->eulr_.yaw) * 300000.0f;
  tmp->data3_symbol = this->eulr_.yaw > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = (int16_t *)send_buff.data;
  tmp[1] = this->eulr_.pit / M_2PI * (float)INT16_MAX;
  tmp[2] = this->eulr_.rol / M_2PI * (float)INT16_MAX;
  tmp[3] = this->eulr_.yaw / M_2PI * (float)INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::EULR_DATA_ID;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendQuat() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData4 *tmp =
      reinterpret_cast<Device::WearLab::CanData4 *>(send_buff.data);

  header.data.device_id = IMU_SEND_CAN_ID;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::QUAT_DATA_ID;
  send_buff.index = header.raw;
  tmp->data[0] = this->quat_.q0 * INT16_MAX;
  tmp->data[1] = this->quat_.q1 * INT16_MAX;
  tmp->data[2] = this->quat_.q2 * INT16_MAX;
  tmp->data[3] = this->quat_.q3 * INT16_MAX;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#endif
}
