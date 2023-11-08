#pragma once

#include <module.hpp>

#include "dev_can.hpp"

namespace Module {
class CanIMU {
 public:
  typedef struct __attribute__((packed)) {
    uint32_t id;
    Component::Type::Quaternion quat_;
    Component::Type::Vector3 gyro_;
    Component::Type::Vector3 accl_;
    uint16_t res;
  } Data;

  CanIMU();

  void SendData();

  static int SetCMD(CanIMU *imu, int argc, char **argv);

 private:
  Data data_{};

  System::Database::Key<uint32_t> delay_;

  System::Database::Key<uint32_t> can_id_;

  System::Term::Command<CanIMU *> cmd_;

  Message::Topic<Data> wl_imu_data_;

  Message::Topic<Data>::RemoteData remote_data_;

  System::Thread thread_;
};
}  // namespace Module
