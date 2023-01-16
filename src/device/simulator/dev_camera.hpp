#include <device.hpp>

#include "webots/robot.h"

namespace Device {
class Camera {
 public:
  Camera();

  WbDeviceTag handle_;
};
}  // namespace Device
