#include <device.hpp>

#include "webots/robot.h"

namespace Device {
class IMU {
 public:
  typedef struct {
    const char* tp_name_prefix;
  } Param;

  IMU(Param& param);

  void Update();

  static int ShowCMD(IMU* imu, int argc, char* argv[]);

  Param param_;

  Message::Topic<Component::Type::Vector3> accl_tp_;
  Message::Topic<Component::Type::Vector3> gyro_tp_;
  Message::Topic<Component::Type::Eulr> eulr_tp_;

  WbDeviceTag ahrs_handle_;
  WbDeviceTag gyro_handle_;
  WbDeviceTag accl_handle_;

  Component::Type::Vector3 accl_;
  Component::Type::Vector3 gyro_;
  Component::Type::Eulr eulr_;

  System::Term::Command<IMU*> cmd_;

  System::Thread thread_;
};
}  // namespace Device
