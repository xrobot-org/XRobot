#include "mod_custom_controller.hpp"

#include "bsp_uart.h"

using namespace Module;

CustomController::CustomController() {
  auto imu_thread = [](CustomController *imu) {
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");
    auto quat_sub =
        Message::Subscriber<Component::Type::Quaternion>("imu_quat");
    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");
    auto accl_sub = Message::Subscriber<Component::Type::Vector3>("imu_accl");

    while (1) {
      eulr_sub.DumpData(imu->eulr_);
      imu->SendEulr();
      imu->thread_.SleepUntil(2);
    }
  };
  this->thread_.Create(imu_thread, this, "imu_thread", 512,
                       System::Thread::MEDIUM);
}
void CustomController::SendEulr() {
  this->uart_trans_buff_.start_frame = START;

  this->uart_trans_buff_.data[0] = this->eulr_.pit / M_2PI * INT16_MAX;
  this->uart_trans_buff_.data[1] = this->eulr_.rol / M_2PI * INT16_MAX;
  this->uart_trans_buff_.data[2] = this->eulr_.yaw / M_2PI * INT16_MAX;

  this->uart_trans_buff_.end_frame = END;

  bsp_uart_transmit(BSP_UART_AI,
                    reinterpret_cast<uint8_t *>(&this->uart_trans_buff_),
                    sizeof(UartData), false);
}
