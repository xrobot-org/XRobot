#include "thread.hpp"
#include "application.hpp"
#include "hardware_container.hpp"
#include "peripheral_manager.hpp"
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

class ExampleApp : public XRobot::Application {
public:
  void OnInit(PeripheralManager &pm) {
    uart_ = pm.FindOrExit<ExampleUART>("uart_console");
    debug_uart_ = pm.FindOrExit<ExampleUART>("debug_uart");
    can_main_ = pm.FindOrExit<ExampleCAN>("can_bus_main");
    can_backup_ = pm.FindOrExit<ExampleCAN>("can_backup");
    std::cout << "Application Init" << std::endl;
  }

  void OnMonitor() {
    uart_->Send("UART Console Start");
    can_main_->AddMessage(0x123);
    can_backup_->AddMessage(0x456);
    debug_uart_->Send("Debug UART Start");
  }

private:
  ExampleUART *uart_ = nullptr;
  ExampleUART *debug_uart_ = nullptr;
  ExampleCAN *can_main_ = nullptr;
  ExampleCAN *can_backup_ = nullptr;
};

#define XR_HARDWARE_BINDINGS                                                   \
  XRobot::MakeEntry(uart_console, "uart_console"),                             \
      XRobot::MakeEntry(debug_uart, "debug_uart"),                             \
      XRobot::MakeEntry(can_main, "can_bus_main"),                             \
      XRobot::MakeEntry(can_backup, "can_backup")

int main() {
  ExampleUART uart_console;
  ExampleUART debug_uart;
  ExampleCAN can_main;
  ExampleCAN can_backup;

  PeripheralManager pm(XRobot::HardwareContainer{XR_HARDWARE_BINDINGS});

  ExampleApp app;

  XRobot::ApplicationManager am;
  am.RegisterApplication(app);
  am.InitAll(pm);
  while (true) {
    am.MonitorAll();
    LibXR::Thread::Sleep(1000);
  }

  return 0;
}
