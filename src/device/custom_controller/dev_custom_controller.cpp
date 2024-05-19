#include "dev_custom_controller.hpp"

#include "bsp_uart.h"

#define REF_LEN_RX_BUFF (0xFF)

using namespace Device;

static std::array<uint8_t, REF_LEN_RX_BUFF> rxbuf;

CustomController::CustomController(Param &param)
    : param_(param), event_(Message::Event::FindEvent("cmd_event")) {
  auto rx_cplt_callback = [](void *arg) {
    CustomController *cust_ctrl = static_cast<CustomController *>(arg);
    cust_ctrl->packet_recv_.Post();
  };

  bsp_uart_register_callback(BSP_UART_EXT, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);
  auto controller_recv_thread = [](CustomController *cust_ctrl) {
    while (1) {
      bsp_uart_receive(BSP_UART_EXT, &rxbuf[0], sizeof(rxbuf), false);
      if (cust_ctrl->packet_recv_.Wait(500)) {
        cust_ctrl->Prase();
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
  return bsp_uart_receive(BSP_UART_EXT, &rxbuf[0], REF_LEN_RX_BUFF, false) ==
         BSP_OK;
}

void CustomController::Prase() {
  auto len = bsp_uart_get_count(BSP_UART_EXT);
  uint8_t *index = &rxbuf[0];

  for (uint32_t i = 0; i < len; i++) {
    if (index[i] == 0xa5 && index[i + 5] == 0x02 && index[i + 6] == 0x03 &&
        len - i > 37) {
      online_ = true;

      memcpy(&data_, index + i, sizeof(data_));
      for (int i = 0; i < 6; i++) {
        if (data_.angle[0] != 0) {
          data_.angle[i] += param_.offset[i];
        } else {
          online_ = false;
        }
      }
      i += 37;
      last_online_time_ = bsp_time_get_ms();
    } else {
      continue;
    }
  }

  if (bsp_time_get_ms() - last_online_time_ > 500) {
    online_ = false;
  }
}

void CustomController::Offline() { online_ = false; }
