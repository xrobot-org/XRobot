#include "mod_custom_controller.hpp"

#include <thread.hpp>

#include "bsp_uart.h"

using namespace Module;

CustomController::CustomController() {
  auto imu_thread = [](CustomController *imu) {
    auto eulr_sub = Message::Subscriber("imu_eulr", imu->eulr_);
    auto quar_sub = Message::Subscriber("imu_quat", imu->quat_);
    auto gyro_sub = Message::Subscriber("imu_gyro", imu->gyro_);
    auto accl_sub = Message::Subscriber("imu_accl", imu->accl_);

    while (1) {
      eulr_sub.DumpData();
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
