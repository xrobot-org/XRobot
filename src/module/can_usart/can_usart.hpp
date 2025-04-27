#include "dev_can.hpp"
#include "module.hpp"

namespace Module {
class CantoUsart {
 public:
  enum FRAME { START = 0xa5, END = 0Xe3 };

  struct __attribute__((packed)) UartData {
    uint8_t start_frame;
    uint32_t id : 31;
    uint8_t type : 1;
    uint8_t data[8];
    uint8_t end_frame;
  };

  CantoUsart();

 private:
  std::array<uint8_t, sizeof(UartData) * 2> uart_recv_buff_1_;
  std::array<uint8_t, sizeof(UartData) * 2> uart_recv_buff_2_;
  std::array<uint8_t, sizeof(UartData) * 2>* uart_recv_buff_addr_;
  UartData uart_trans_buff_;
  System::Semaphore tx_cplt_;
  System::Semaphore tx_ready_;
};
}  // namespace Module
