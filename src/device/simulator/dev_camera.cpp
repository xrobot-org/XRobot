#include "dev_camera.hpp"

#include "webots/camera.h"

using namespace Device;

Camera::Camera() {
  this->handle_ = wb_robot_get_device("camera");
  wb_camera_enable(this->handle_, 10);
}
