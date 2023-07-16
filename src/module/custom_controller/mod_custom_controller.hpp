#include "dev_bmi088.hpp"
#include "module.hpp"

namespace Module {
class CustomController {
 public:
  CustomController();

  void SendAccl();

  void SendGyro();

  void SendEulr();

  void SendQuat();

 private:
  enum FRAME { START = 0xa5, END = 0Xe3 };

  struct __attribute__((packed)) UartData {
    uint8_t start_frame;
    uint8_t data[3];
    uint8_t end_frame;
  };

  Component::Type::Eulr eulr_;
  Component::Type::Quaternion quat_;
  Component::Type::Vector3 gyro_;
  Component::Type::Vector3 accl_;

  System::Thread thread_;

  UartData uart_trans_buff_;
};
}  // namespace Module
