#include "mod_canfd_imu.hpp"

#include <comp_crc8.hpp>

#include "bsp_can.h"
#include "bsp_time.h"
#include "bsp_uart.h"
#include "dev_can.hpp"
#include "ms.h"

using namespace Module;

CanfdImu::CanfdImu()
    : data_tp_("canfd_imu"),
      id_("canfd_id", 0x30),
      fb_cycle_("canfd_fb_cycle", 2),
      accl_enable_("accl_enable", true),
      gyro_enable_("gyro_enable", true),
      magn_enable_("magn_enable", true),
      quat_enable_("quat_enable", true),
      eulr_enable_("eulr_enable", true),
      canfd_enable_("canfd_enable", false),
      raw_magn_("raw_magn_enable", false),
      cmd_(this, SetCMD, "set_imu") {
  auto cali_cb = [](Device::Can::FDPack &pack, CanfdImu *imu) {
    if (pack.info.size != sizeof(ControlData)) {
      return false;
    }

    auto cali_data = reinterpret_cast<ControlData *>(pack.info.data);

    imu->cali_gyro_ = cali_data->cali_gyro;
    imu->cali_magn_ = cali_data->cali_magn;

    return true;
  };

  auto cali_tp = new Message::Topic<Device::Can::FDPack>("imu_cali");

  Device::WearLab::CanHeader tmp;
  tmp.data = {.device_type = Device::WearLab::DEV_TYPE_IMU_9,
              .data_type = Device::WearLab::IMU_9_CALI_DATA,
              .device_id = id_,
              .res = 0};

  cali_tp->RegisterCallback(cali_cb, this);

  Device::Can::SubscribeFD(*cali_tp, BSP_CAN_1, tmp.raw, 1);

  auto thread_fn = [](CanfdImu *imu) {
    auto magn_sub = Message::Subscriber<Component::Type::Vector3>("magn");
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");
    auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl");
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");
    auto raw_magn_sub =
        Message::Subscriber<Component::Type::Vector3>("raw_magn");

    uint32_t last_wakeup = bsp_time_get_ms();

    while (true) {
      accl_sub.DumpData(imu->data_.raw.accl_);
      gyro_sub.DumpData(imu->data_.raw.gyro_);
      quat_sub.DumpData(imu->data_.raw.quat_);
      eulr_sub.DumpData(imu->eulr_);
      imu->data_.raw.eulr_.pit = imu->eulr_.pit;
      imu->data_.raw.eulr_.rol = imu->eulr_.rol;
      imu->data_.raw.eulr_.yaw = imu->eulr_.yaw;

      if (imu->raw_magn_) {
        raw_magn_sub.DumpData(imu->data_.raw.magn_);
      } else {
        magn_sub.DumpData(imu->data_.raw.magn_);
      }

      if (imu->canfd_enable_) {
        Device::Can::SendFDExtPack(BSP_CAN_1, imu->id_,
                                   reinterpret_cast<uint8_t *>(&imu->data_.raw),
                                   sizeof(imu->data_.raw));
      } else {
        if (imu->accl_enable_) {
          imu->SendAccl();
        }
        if (imu->gyro_enable_) {
          imu->SendGyro();
        }
        if (imu->magn_enable_) {
          imu->SendAccl();
        }
        if (imu->quat_enable_) {
          imu->SendAccl();
        }
        if (imu->eulr_enable_) {
          imu->SendAccl();
        }
      }

      imu->data_.prefix = 0xa5;
      imu->data_.id = imu->id_;
      imu->data_.crc8 = Component::CRC8::Calculate(
          reinterpret_cast<const uint8_t *>(&imu->data_),
          sizeof(imu->data_) - sizeof(uint8_t), CRC8_INIT);
      bsp_uart_transmit(BSP_UART_MCU, reinterpret_cast<uint8_t *>(&imu->data_),
                        sizeof(Data), false);

      imu->thread_.SleepUntil(imu->fb_cycle_, last_wakeup);
    }
  };

  this->thread_.Create(thread_fn, this, "canfd_imu", 384,
                       System::Thread::REALTIME);
}

int CanfdImu::SetCMD(CanfdImu *imu, int argc, char **argv) {
  printf("\n");
  if (argc == 1) {
    printf("set_delay  [time]       设置发送延时ms\r\n");
    printf("set_can_id [id]         设置can id\r\n");
    printf("enable/disable     [accl/gyro/magn/raw_magn/quat/eulr/canfd]\r\n");
  } else if (argc == 3 && strcmp(argv[1], "set_delay") == 0) {
    int delay = std::stoi(argv[2]);

    if (delay > 1000) {
      delay = 1000;
    }

    if (delay < 1) {
      delay = 1;
    }

    imu->fb_cycle_.Set(delay);

    printf("delay:%d\r\n", delay);
  } else if (argc == 3 && strcmp(argv[1], "enable") == 0) {
    if (strcmp(argv[2], "accl") == 0) {
      imu->accl_enable_.Set(true);
    } else if (strcmp(argv[2], "gyro") == 0) {
      imu->gyro_enable_.Set(true);
    } else if (strcmp(argv[2], "magn") == 0) {
      imu->magn_enable_.Set(true);
    } else if (strcmp(argv[2], "quat") == 0) {
      imu->quat_enable_.Set(true);
    } else if (strcmp(argv[2], "eulr") == 0) {
      imu->eulr_enable_.Set(true);
    } else if (strcmp(argv[2], "raw_magn") == 0) {
      imu->raw_magn_.Set(true);
    }

  } else if (argc == 3 && strcmp(argv[1], "disable") == 0) {
    if (strcmp(argv[2], "accl") == 0) {
      imu->accl_enable_.Set(false);
    } else if (strcmp(argv[2], "gyro") == 0) {
      imu->gyro_enable_.Set(false);
    } else if (strcmp(argv[2], "magn") == 0) {
      imu->magn_enable_.Set(false);
    } else if (strcmp(argv[2], "quat") == 0) {
      imu->quat_enable_.Set(false);
    } else if (strcmp(argv[2], "eulr") == 0) {
      imu->eulr_enable_.Set(false);
    } else if (strcmp(argv[2], "raw_magn") == 0) {
      imu->raw_magn_.Set(false);
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
