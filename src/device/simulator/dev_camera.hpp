#include "comp_utils.hpp"
#include "dev.hpp"
#include "webots/robot.h"

namespace Device {
class Camera {
 public:
  Camera();

  WbDeviceTag handle_;
};
}  // namespace Device
