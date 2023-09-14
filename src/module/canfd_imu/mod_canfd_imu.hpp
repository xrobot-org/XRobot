#include <database.hpp>

#include "module.hpp"

namespace Module {
class CanfdImu {
 public:
  typedef union {
    struct __attribute__((packed)) {
      uint32_t device_type : 12;
      uint32_t data_type : 6;
      uint32_t device_id : 6;
      uint32_t res : 8;
    } data;

    uint32_t raw;
  } CanHeader;

  typedef struct __attribute__((packed)) {
    uint32_t time;
    Component::Type::Quaternion quat_;
    Component::Type::Vector3 gyro_;
    Component::Type::Vector3 accl_;
    Component::Type::Vector3 magn_;
  } Data;

  Data data_;

  CanHeader header_;

  Message::Topic<Data> data_tp_;

  System::Thread thread_;

  System::Database::Key<uint32_t> id_;

  System::Database::Key<uint32_t> cycle_;

  System::Term::Command<CanfdImu*> cmd_;

  CanfdImu();

  static int SetCMD(CanfdImu* imu, int argc, char** argv);
};
}  // namespace Module
