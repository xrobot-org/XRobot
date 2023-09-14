#include "mod_canfd_imu.hpp"

#include <thread.hpp>

#include "bsp_can.h"
#include "bsp_time.h"
#include "dev_canfd.hpp"

using namespace Module;

CanfdImu::CanfdImu()
    : data_tp_("canfd_imu"),
      id_("canfd_imu_id", 0x30),
      cycle_("canfd_imu_cycle", 10),
      cmd_(this, SetCMD, "set_imu") {
  auto thread_fn = [](CanfdImu *imu) {
    Message::Subscriber accl_sub("imu_accl", imu->data_.accl_);
    Message::Subscriber gyro_sub("imu_gyro", imu->data_.gyro_);
    Message::Subscriber magn_sub("magn", imu->data_.magn_);
    Message::Subscriber quat_sub("imu_quat", imu->data_.quat_);

    uint32_t last_wakeup = bsp_time_get_ms();

    while (true) {
      accl_sub.DumpData();
      gyro_sub.DumpData();
      magn_sub.DumpData();
      quat_sub.DumpData();

      imu->header_.data = {
          .device_type = 0x01, .data_type = 0x01, .device_id = imu->id_};

      imu->data_.time = bsp_time_get_ms();

      Device::Can::SendFDExtPack(BSP_CAN_1, imu->header_.raw,
                                 reinterpret_cast<uint8_t *>(&imu->data_),
                                 sizeof(imu->data_));

      imu->thread_.SleepUntil(imu->cycle_, last_wakeup);
    }
  };

  this->thread_.Create(thread_fn, this, "canfd_imu", 384,
                       System::Thread::REALTIME);
}

int CanfdImu::SetCMD(CanfdImu *imu, int argc, char **argv) {
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

    imu->cycle_.Set(delay);

    printf("delay:%d\r\n", delay);
  } else if (argc == 3 && strcmp(argv[1], "set_can_id") == 0) {
    int id = std::stoi(argv[2]);

    imu->id_.Set(id);

    printf("can_id:%d\r\n", id);
  } else {
    printf("命令错误\r\n");
  }

  return 0;
}
