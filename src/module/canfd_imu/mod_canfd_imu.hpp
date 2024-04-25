#include <database.hpp>

#include "dev_can.hpp"
#include "module.hpp"

namespace Module {
class CanfdImu {
 public:
  typedef struct __attribute__((packed)) {
    uint8_t prefix;
    uint8_t id;
    struct __attribute__((packed)) {
      Component::Type::Quaternion quat_;
      Component::Type::Vector3 gyro_;
      Component::Type::Vector3 accl_;
      Component::Type::Vector3 magn_;
      struct __attribute__((packed)) {
        float yaw; /* 偏航角（Yaw angle） */
        float pit; /* 俯仰角（Pitch angle） */
        float rol; /* 翻滚角（Roll angle） */
      } eulr_;
    } raw;
    uint8_t crc8;
  } Data;

  typedef struct __attribute__((packed)) {
    uint32_t cali_magn;
    uint32_t cali_gyro;
  } ControlData;

  bool cali_magn_ = false, cali_gyro_ = false;

  Data data_;

  Message::Topic<Data> data_tp_;

  System::Thread thread_;

  Component::Type::Eulr eulr_;

  System::Database::Key<uint32_t> id_;

  System::Database::Key<uint32_t> fb_cycle_;

  System::Database::Key<bool> accl_enable_;
  System::Database::Key<bool> gyro_enable_;
  System::Database::Key<bool> magn_enable_;
  System::Database::Key<bool> quat_enable_;
  System::Database::Key<bool> eulr_enable_;
  System::Database::Key<bool> canfd_enable_;
  System::Database::Key<bool> raw_magn_;

  System::Term::Command<CanfdImu *> cmd_;

  Device::Can::Pack send_buff;

  CanfdImu();

  void SendAccl() {
    int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
    tmp[0] = this->data_.raw.accl_.x / 32.0f * INT16_MAX;
    tmp[1] = this->data_.raw.accl_.y / 32.0f * INT16_MAX;
    tmp[2] = this->data_.raw.accl_.z / 32.0f * INT16_MAX;
    send_buff.index = this->id_.data_;
    Device::Can::SendStdPack(BSP_CAN_1, send_buff);
  }

  void SendGyro() {
    int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
    tmp[0] = this->data_.raw.gyro_.x / 34.90658502f / 2.0f * UINT16_MAX;
    tmp[1] = this->data_.raw.gyro_.y / 34.90658502f / 2.0f * UINT16_MAX;
    tmp[2] = this->data_.raw.gyro_.z / 34.90658502f / 2.0f * UINT16_MAX;
    send_buff.index = this->id_.data_ + 1;
    Device::Can::SendStdPack(BSP_CAN_1, send_buff);
  }

  void SendMagn() {
    int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
    tmp[0] = this->data_.raw.magn_.x / 2.0f * UINT16_MAX;
    tmp[1] = this->data_.raw.magn_.y / 2.0f * UINT16_MAX;
    tmp[2] = this->data_.raw.magn_.z / 2.0f * UINT16_MAX;
    send_buff.index = this->id_.data_ + 1;
    Device::Can::SendStdPack(BSP_CAN_1, send_buff);
  }

  void SendEulr() {
    int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
    tmp[0] = this->data_.raw.eulr_.pit / M_2PI * INT16_MAX;
    tmp[1] = this->data_.raw.eulr_.rol / M_2PI * INT16_MAX;
    tmp[2] = this->data_.raw.eulr_.yaw / M_2PI * INT16_MAX;
    send_buff.index = this->id_.data_ + 2;
    Device::Can::SendStdPack(BSP_CAN_1, send_buff);
  }

  void SendQuat() {
    int16_t *tmp = reinterpret_cast<int16_t *>(send_buff.data);
    tmp[0] = this->data_.raw.quat_.q0 / 2.0f * INT16_MAX;
    tmp[1] = this->data_.raw.quat_.q1 / 2.0f * INT16_MAX;
    tmp[2] = this->data_.raw.quat_.q2 / 2.0f * INT16_MAX;
    tmp[3] = this->data_.raw.quat_.q3 / 2.0f * INT16_MAX;

    send_buff.index = this->id_.data_ + 3;
    Device::Can::SendStdPack(BSP_CAN_1, send_buff);
  }

  static int SetCMD(CanfdImu *imu, int argc, char **argv);
};
}  // namespace Module
