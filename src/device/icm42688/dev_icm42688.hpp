#pragma once

#include <database.hpp>
#include <device.hpp>

#include "bsp_gpio.h"
#include "dev_ahrs.hpp"

namespace Device {
class ICM42688 {
 public:
  typedef struct {
    Component::Type::Vector3 gyro_offset; /* 陀螺仪偏置 */
  } Calibration;                          /* 校准数据 */

  typedef struct {
    /* 旋转矩阵 */
    float rot_mat[3][3];
  } Rotation;

  ICM42688(ICM42688::Rotation &rot);

  bool Init();

  void Prase();

  bool StartRecv();

  void Select() { bsp_gpio_write_pin(BSP_GPIO_IMU_CS, false); }

  void Unselect() { bsp_gpio_write_pin(BSP_GPIO_IMU_CS, true); }

  void WriteSingle(uint8_t reg, uint8_t data);

  uint8_t ReadSingle(uint8_t reg);

  void Read(uint8_t reg, uint8_t *data, uint8_t len);

  static int CaliCMD(ICM42688 *icm42688, int argc, char **argv);

 private:
  System::Database::Key<Calibration> cali_;
  Rotation &rot_;

  System::Semaphore raw_;
  System::Semaphore new_;

  float temp_ = 0.0f; /* 温度 */

  System::Thread thread_accl_, thread_gyro_;

  Message::Topic<Component::Type::Vector3> accl_tp_;
  Message::Topic<Component::Type::Vector3> gyro_tp_;

  Component::Type::Vector3 accl_{};
  Component::Type::Vector3 gyro_{};

  System::Term::Command<ICM42688 *> cmd_;
};
}  // namespace Device
