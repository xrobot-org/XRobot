#include "mod_can_imu.hpp"

#include <comp_type.hpp>

#include "dev_can.hpp"

using namespace Module;

static Device::Can::Pack send_buff;

CanIMU::CanIMU()
    : enable_eulr_("enable_eulr", true),
      enable_quat_("enable_quat", true),
      enable_gyro_("enable_gyro", true),
      enable_accl_("enable_accl", true),
      delay_("imu_thread_delay", 10),
      can_id_("imu_can_id", 0x00),
      cmd_(this, SetCMD, "set_imu") {
  auto imu_thread = [](CanIMU *imu) {
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");
    auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      if (imu->enable_accl_.data_) {
        accl_sub.DumpData(imu->accl_);
        imu->SendAccl();
      }

      if (imu->enable_gyro_.data_) {
        gyro_sub.DumpData(imu->gyro_);
        imu->SendGyro();
      }

      if (imu->enable_eulr_.data_) {
        eulr_sub.DumpData(imu->eulr_);
        imu->SendEulr();
      }

      if (imu->enable_quat_.data_) {
        quat_sub.DumpData(imu->quat_);
        imu->SendQuat();
      }

      imu->thread_.SleepUntil(imu->delay_.data_, last_online_time);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread",
                       MODULE_CAN_IMU_TASK_STACK_DEPTH, System::Thread::MEDIUM);
}

void CanIMU::SendAccl() {
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->accl_.x / 6.0f * INT16_MAX;
  tmp[2] = this->accl_.y / 6.0f * INT16_MAX;
  tmp[3] = this->accl_.z / 6.0f * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::ACCL_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
}

void CanIMU::SendGyro() {
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->gyro_.x / 20.0f * INT16_MAX;
  tmp[2] = this->gyro_.y / 20.0f * INT16_MAX;
  tmp[3] = this->gyro_.z / 20.0f * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::GYRO_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
}

void CanIMU::SendEulr() {
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->eulr_.pit / M_2PI * INT16_MAX;
  tmp[2] = this->eulr_.rol / M_2PI * INT16_MAX;
  tmp[3] = this->eulr_.yaw / M_2PI * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::EULR_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
}

void CanIMU::SendQuat() {}

int CanIMU::SetCMD(CanIMU *imu, int argc, char **argv) {
  if (argc == 1) {
    printf("set_delay  [time]  设置发送延时ms\r\n");
    printf("enable     [accl/gyro/eulr/quat] 开启功能\r\n");
    printf("disable    [accl/gyro/eulr/quat] 关闭功能\r\n");
    printf("set_can_id [id] 设置can id\r\n");
  } else if (argc == 3 && (strcmp(argv[1], "enable") == 0 ||
                           strcmp(argv[1], "disable") == 0)) {
    bool enable = (strcmp(argv[1], "enable") == 0);

    if (strcmp(argv[2], "accl") == 0) {
      imu->enable_accl_.Set(enable);
    } else if (strcmp(argv[2], "gyro") == 0) {
      imu->enable_gyro_.Set(enable);
    } else if (strcmp(argv[2], "eulr") == 0) {
      imu->enable_eulr_.Set(enable);
    } else if (strcmp(argv[2], "quat") == 0) {
      imu->enable_quat_.Set(enable);
    } else {
      printf("命令错误\r\n");
    }
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

    imu->can_id_.Set(id);

    printf("can_id:%d\r\n", id);
  } else {
    printf("命令错误\r\n");
  }

  return 0;
}
