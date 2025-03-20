#include "xrobot/hardware_container.hpp"
#include "xrobot/platform_adapter.hpp"
#include <iostream>

struct ExampleUART {
  void Send(const char *msg) {
    std::cout << "[UART] Sent: " << msg << std::endl;
  }
};

struct ExampleCAN {
  void AddMessage(int id) {
    std::cout << "[CAN] Added message with ID: " << id << std::endl;
  }
};

#define XR_HARDWARE_BINDINGS                                                   \
  xrobot::MakeEntry(uart_console, "uart_console"),                             \
      xrobot::MakeEntry(debug_uart, "debug_uart"),                             \
      xrobot::MakeEntry(can_main, "can_bus_main"),                             \
      xrobot::MakeEntry(can_backup, "can_backup")

int main() {
  ExampleUART uart_console;
  ExampleUART debug_uart;
  ExampleCAN can_main;
  ExampleCAN can_backup;

  xrobot::HardwareContainer container(XR_HARDWARE_BINDINGS);
  xrobot::PlatformAdapter::Init(container);

  auto *uart = PeripheralManager::Instance().Find<ExampleUART>("uart_console");
  if (uart) {
    uart->Send("XRobot2.0 Framework Start");
  } else {
    std::cout << "UART not found." << std::endl;
  }

  return 0;
}