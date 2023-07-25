#include "dev_custom_controller.hpp"

#include "bsp_uart.h"

#define REF_LEN_RX_BUFF (0xFF)

using namespace Device;

static std::array<uint8_t, REF_LEN_RX_BUFF> rxbuf;

CustomController::CustomController()
    : event_(Message::Event::FindEvent("cmd_event")) {
  auto rx_cplt_callback = [](void *arg) {
    CustomController *cust_ctrl = static_cast<CustomController *>(arg);
    cust_ctrl->packet_recv_.Post();
  };

  bsp_uart_register_callback(BSP_UART_EXT, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);
  Component::CMD::RegisterController(this->controller_angel_);
  auto controller_recv_thread = [](CustomController *cust_ctrl) {
    while (1) {
      if (cust_ctrl->packet_recv_.Wait(20)) {
        cust_ctrl->Prase();
        cust_ctrl->controller_angel_.Publish(cust_ctrl->controller_data_);
      } else {
        cust_ctrl->Offline();
      }
    }
  };
  this->recv_thread_.Create(controller_recv_thread, this,
                            "controller_recv_thread", 256,
                            System::Thread::REALTIME);
}

bool CustomController::StartRecv() {
  return bsp_uart_receive(BSP_UART_EXT, rxbuf, REF_LEN_RX_BUFF, false) ==
         BSP_OK;
}

void CustomController::Prase() {
  this->controller_angel_.Publish(this->controller_data_);
}
void CustomController::Offline() {
  this->controller_data_.online = false;
  memset(&(this->controller_data_), 0, sizeof(this->controller_data_));
  this->controller_angel_.Publish(this->controller_data_);
};
