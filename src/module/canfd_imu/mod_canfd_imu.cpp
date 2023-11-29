#include "mod_canfd_imu.hpp"

#include <comp_crc8.hpp>

#include "bsp_can.h"
#include "bsp_time.h"
#include "bsp_uart.h"
#include "dev_canfd.hpp"

using namespace Module;

CanfdImu::CanfdImu()
    : data_tp_("canfd_imu"),
      uart_output_("imu_uart", true),
      canfd_output_("imu_canfd", true),
      id_("canfd_imu_id", 0x30),
      cycle_("canfd_imu_cycle", 10),
      cmd_(this, SetCMD, "set_imu") {
  auto cmd_cb = [](Device::Can::FDPack &pack, CanfdImu *imu) {
    if (pack.info.size != sizeof(ControlData)) {
      return false;
    }

    ControlData *data = reinterpret_cast<ControlData *>(pack.info.data);
    if (data->id != imu->id_) {
      imu->id_.Set(data->id);
      bsp_sys_reset();
    }
    if (data->cycle != imu->cycle_) {
      imu->cycle_.Set(data->cycle);
    }
    if (static_cast<bool>(data->uart_output) != imu->uart_output_) {
      imu->id_.Set(data->uart_output);
    }
    if (static_cast<bool>(data->canfd_output) != imu->canfd_output_) {
      imu->id_.Set(data->canfd_output);
    }
    return true;
  };

  auto cmd_tp = new Message::Topic<Device::Can::FDPack>("imu_cmd");

  CanHeader tmp;
  tmp.data = {.device_type = IMU_9, .data_type = CONTROL, .device_id = id_};

  cmd_tp->RegisterCallback(cmd_cb, this);

  Device::Can::SubscribeFD(*cmd_tp, BSP_CAN_1, tmp.raw, 1);

  auto thread_fn = [](CanfdImu *imu) {
    auto magn_sub = Message::Subscriber<Component::Type::Vector3>("magn");
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");
    auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl");

    uint32_t last_wakeup = bsp_time_get_ms();

    while (true) {
      accl_sub.DumpData(imu->data_.raw.accl_);
      gyro_sub.DumpData(imu->data_.raw.gyro_);
      magn_sub.DumpData(imu->data_.raw.magn_);
      quat_sub.DumpData(imu->data_.raw.quat_);

      imu->header_.data = {
          .device_type = IMU_9, .data_type = FEEDBACK, .device_id = imu->id_};

      imu->data_.raw.time = bsp_time_get_ms();

      if (imu->canfd_output_) {
        Device::Can::SendFDExtPack(BSP_CAN_1, imu->header_.raw,
                                   reinterpret_cast<uint8_t *>(&imu->data_.raw),
                                   sizeof(imu->data_.raw));
      }

      if (imu->uart_output_) {
        imu->data_.prefix = 0xa5;
        imu->data_.id = imu->id_;
        imu->data_.crc8 = Component::CRC8::Calculate(
            reinterpret_cast<const uint8_t *>(&imu->data_),
            sizeof(imu->data_) - sizeof(uint8_t), CRC8_INIT);
        bsp_uart_transmit(BSP_UART_MCU,
                          reinterpret_cast<uint8_t *>(&imu->data_),
                          sizeof(Data), false);
      }
      imu->thread_.SleepUntil(imu->cycle_, last_wakeup);
    }
  };

  this->thread_.Create(thread_fn, this, "canfd_imu", 384,
                       System::Thread::REALTIME);
}

int CanfdImu::SetCMD(CanfdImu *imu, int argc, char **argv) {
  imu->uart_output_.data_ = false;
  printf("\n");
  if (argc == 1) {
    printf("set_delay  [time]       设置发送延时ms\r\n");
    printf("set_can_id [id]         设置can id\r\n");
    printf("enable     [uart/canfd] 开启输出\r\n");
    printf("disable    [uart/canfd] 关闭输出\r\n");
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
  } else if (argc == 3 && strcmp(argv[1], "enable") == 0) {
    if (strcmp(argv[2], "uart") == 0) {
      imu->uart_output_.Set(true);
    } else if (strcmp(argv[2], "canfd") == 0) {
      imu->canfd_output_.Set(true);
    }
  } else if (argc == 3 && strcmp(argv[1], "disable") == 0) {
    if (strcmp(argv[2], "uart") == 0) {
      imu->uart_output_.Set(false);
    } else if (strcmp(argv[2], "canfd") == 0) {
      imu->canfd_output_.Set(false);
    }
  } else if (argc == 3 && strcmp(argv[1], "set_can_id") == 0) {
    int id = std::stoi(argv[2]);

    imu->id_.Set(id);

    printf("can_id:%d\r\n", id);
  } else {
    printf("命令错误\r\n");
  }

  return 0;
}
