#include "mod_can_imu.hpp"

#include "dev_can.hpp"
#include "ms.h"

using namespace Module;

static Device::Can::Pack send_buff;

#if IMU_USE_IN_WEARLAB
static Device::WearLab::CanHeader header;
#endif

CanIMU::CanIMU()
    : enable_eulr_("enable_eulr", true),
      enable_quat_("enable_quat", true),
      enable_gyro_("enable_gyro", true),
      enable_accl_("enable_accl", true),
      delay_("imu_thread_delay", 10),
      can_id_("imu_can_id", 0x00),
      cmd_(this, SetCMD, "set_imu") {
  auto imu_thread = [](CanIMU *imu) {
    auto eulr_sub = Message::Subscriber("imu_eulr", imu->eulr_);
    auto quar_sub = Message::Subscriber("imu_quat", imu->quat_);
    auto gyro_sub = Message::Subscriber("imu_gyro", imu->gyro_);
    auto accl_sub = Message::Subscriber("imu_accl", imu->accl_);

    while (1) {
      if (imu->enable_accl_.data_) {
        accl_sub.DumpData();
        imu->SendAccl();
      }

      if (imu->enable_gyro_.data_) {
        gyro_sub.DumpData();
        imu->SendGyro();
      }

      if (imu->enable_eulr_.data_) {
        eulr_sub.DumpData();
        imu->SendEulr();
      }

      if (imu->enable_quat_.data_) {
        quar_sub.DumpData();
        imu->SendQuat();
      }

      imu->thread_.SleepUntil(imu->delay_.data_);
    }
  };

  this->thread_.Create(imu_thread, this, "imu_thread",
                       MODULE_CAN_IMU_TASK_STACK_DEPTH, System::Thread::MEDIUM);
}

void CanIMU::SendAccl() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = this->can_id_.data_;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::ACCL_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = static_cast<uint32_t>(fabsf(this->accl_.x) * 100000.0f);
  tmp->data1_symbol = this->accl_.x > 0 ? 0 : 1;
  tmp->data2 = static_cast<uint32_t>(fabsf(this->accl_.y) * 100000.0f);
  tmp->data2_symbol = this->accl_.y > 0 ? 0 : 1;
  tmp->data3 = static_cast<uint32_t>(fabsf(this->accl_.z) * 100000.0f);
  tmp->data3_symbol = this->accl_.z > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->accl_.x / 6.0f * INT16_MAX;
  tmp[2] = this->accl_.y / 6.0f * INT16_MAX;
  tmp[3] = this->accl_.z / 6.0f * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::ACCL_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendGyro() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = this->can_id_.data_;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::GYRO_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = static_cast<uint32_t>(fabsf(this->gyro_.x) * 20000.0f);
  tmp->data1_symbol = this->gyro_.x > 0 ? 0 : 1;
  tmp->data2 = static_cast<uint32_t>(fabsf(this->gyro_.y) * 20000.0f);
  tmp->data2_symbol = this->gyro_.y > 0 ? 0 : 1;
  tmp->data3 = static_cast<uint32_t>(fabsf(this->gyro_.z) * 20000.0f);
  tmp->data3_symbol = this->gyro_.z > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->gyro_.x / 20.0f * INT16_MAX;
  tmp[2] = this->gyro_.y / 20.0f * INT16_MAX;
  tmp[3] = this->gyro_.z / 20.0f * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::GYRO_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendEulr() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData3 *tmp =
      reinterpret_cast<Device::WearLab::CanData3 *>(send_buff.data);
  header.data.device_id = this->can_id_.data_;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::EULR_DATA_ID;
  send_buff.index = header.raw;
  tmp->data1 = static_cast<uint32_t>(fabsf(this->eulr_.pit) * 300000.0f);
  tmp->data1_symbol = this->eulr_.pit > 0 ? 0 : 1;
  tmp->data2 = static_cast<uint32_t>(fabsf(this->eulr_.rol) * 300000.0f);
  tmp->data2_symbol = this->eulr_.rol > 0 ? 0 : 1;
  tmp->data3 = static_cast<uint32_t>(fabsf(this->eulr_.yaw) * 300000.0f);
  tmp->data3_symbol = this->eulr_.yaw > 0 ? 0 : 1;
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#else
  int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
  tmp[1] = this->eulr_.pit / M_2PI * INT16_MAX;
  tmp[2] = this->eulr_.rol / M_2PI * INT16_MAX;
  tmp[3] = this->eulr_.yaw / M_2PI * INT16_MAX;
  send_buff.data[0] = Device::IMU::IMU_DEVICE_ID;
  send_buff.data[1] = Device::IMU::EULR_DATA_ID;
  send_buff.index = this->can_id_.data_;
  Device::Can::SendStdPack(BSP_CAN_1, send_buff);
#endif
}

void CanIMU::SendQuat() {
#if IMU_USE_IN_WEARLAB
  Device::WearLab::CanData4 *tmp =
      reinterpret_cast<Device::WearLab::CanData4 *>(send_buff.data);

  header.data.device_id = this->can_id_.data_;
  header.data.device_type = Device::IMU::IMU_DEVICE_ID;
  header.data.data_type = Device::IMU::QUAT_DATA_ID;
  send_buff.index = header.raw;
  tmp->data[0] = static_cast<int16_t>(this->quat_.q0 * INT16_MAX);
  tmp->data[1] = static_cast<int16_t>(this->quat_.q1 * INT16_MAX);
  tmp->data[2] = static_cast<int16_t>(this->quat_.q2 * INT16_MAX);
  tmp->data[3] = static_cast<int16_t>(this->quat_.q3 * INT16_MAX);
  Device::Can::SendExtPack(BSP_CAN_1, send_buff);
#endif
}

int CanIMU::SetCMD(CanIMU *imu, int argc, char **argv) {
  if (argc == 1) {
    ms_printf("set_delay  [time]  设置发送延时ms");
    ms_enter();
    ms_printf("enable     [accl/gyro/eulr/quat] 开启功能");
    ms_enter();
    ms_printf("disable    [accl/gyro/eulr/quat] 关闭功能");
    ms_enter();
    ms_printf("set_can_id [id] 设置can id");
    ms_enter();
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
      ms_printf("命令错误");
      ms_enter();
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

    ms_printf("delay:%d", delay);
    ms_enter();
  } else if (argc == 3 && strcmp(argv[1], "set_can_id") == 0) {
    int id = std::stoi(argv[2]);

    imu->can_id_.Set(id);

    ms_printf("can_id:%d", id);
    ms_enter();
  } else {
    ms_printf("命令错误");
    ms_enter();
  }

  return 0;
}
