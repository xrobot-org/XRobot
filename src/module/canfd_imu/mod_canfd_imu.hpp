#include <database.hpp>

#include "module.hpp"

namespace Module {
class CanfdImu {
 public:
  typedef enum { IMU_9 = 2 } DeviceType;

  typedef enum { FEEDBACK = 1, CONTROL } DataType;

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
    uint8_t prefix;
    uint8_t id;
    struct __attribute__((packed)) {
      uint32_t time;
      Component::Type::Quaternion quat_;
      Component::Type::Vector3 gyro_;
      Component::Type::Vector3 accl_;
      Component::Type::Vector3 magn_;
    } raw;
    uint8_t crc8;
  } Data;

  typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t cycle;
    uint8_t uart_output;
    uint8_t canfd_output;
    uint16_t res;
  } ControlData;

  Data data_;

  CanHeader header_;

  Message::Topic<Data> data_tp_;

  System::Thread thread_;

  System::Database::Key<bool> uart_output_;

  System::Database::Key<bool> canfd_output_;

  System::Database::Key<uint32_t> id_;

  System::Database::Key<uint32_t> cycle_;

  System::Term::Command<CanfdImu*> cmd_;

  CanfdImu();

  static int SetCMD(CanfdImu* imu, int argc, char** argv);
};
}  // namespace Module
