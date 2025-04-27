#include "mod_can_imu.hpp"

#include "bsp_can.h"
#include "dev_can.hpp"
#include "dev_can_imu.hpp"
#include "wearlab.hpp"

using namespace Module;

#if IMU_USE_IN_WEARLAB
static Device::WearLab::CanHeader header;
#endif

CanIMU::CanIMU()
    : delay_("imu_thread_delay", 10),
      can_id_("imu_can_id", 1),
      cmd_(this, SetCMD, "set_imu"),
      wl_imu_data_("wl_imu_data") {
  auto imu_thread = [](CanIMU *imu) {
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");
    auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      accl_sub.DumpData(imu->data_.accl_);
      gyro_sub.DumpData(imu->data_.gyro_);
      quat_sub.DumpData(imu->data_.quat_);

      imu->SendData();

      imu->thread_.SleepUntil(imu->delay_.data_, last_online_time);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread", 512,
                       System::Thread::MEDIUM);
}
int a = 0;

void CanIMU::SendData() {
  static Device::Can::Pack pack;
  uint8_t *buff = reinterpret_cast<uint8_t *>(&remote_data_);
  static Device::WearLab::CanHeader header;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::ACCL_DATA_ID;
  header.data.device_id = can_id_.data_;
  pack.index = header.raw;
  data_.id = header.raw;
  wl_imu_data_.Publish(data_);
  wl_imu_data_.PackData(remote_data_);
  for (uint32_t i = 0; i < sizeof(Message::Topic<Data>::RemoteData);
       i += sizeof(pack.data)) {
    memcpy(pack.data, buff + i, 8);
    Device::Can::SendExtPack(BSP_CAN_1, pack);
    System::Thread::Sleep(1);
  }
}

int CanIMU::SetCMD(CanIMU *imu, int argc, char **argv) {
  if (argc == 1) {
    printf("set_delay  [time]  设置发送延时ms\r\n");
    printf("set_can_id [id] 设置can id\r\n");
  } else if (argc == 3 && strcmp(argv[1], "set_delay") == 0) {
    int delay = std::stoi(argv[2]);

    if (delay > 1000) {
      delay = 1000;
    }

    if (delay < 1) {
      delay = 1;
    }

    imu->delay_.Set(delay);

    printf("delay:%d\r\n", delay);
  } else if (argc == 3 && strcmp(argv[1], "set_can_id") == 0) {
    int id = std::stoi(argv[2]);

    if (id >= 32) {
      printf("id should be between 0 and 32\r\n");
      return 0;
    }

    imu->can_id_.Set(id);

    printf("can_id:%d\r\n", id);
  } else {
    printf("命令错误\r\n");
  }

  return 0;
}
