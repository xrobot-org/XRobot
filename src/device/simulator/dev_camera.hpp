#include <device.hpp>

#include "webots/robot.h"

namespace Device {
class Camera {
 public:
  Camera();

 private:
  WbDeviceTag handle_;
};
}  // namespace Device
