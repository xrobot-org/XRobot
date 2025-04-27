#include "comp_cmd.hpp"
#include "comp_ui.hpp"
#include "device.hpp"

namespace Device {
class CustomController {
 public:
  typedef struct {
    float offset[6];
  } Param;

  CustomController(Param& param);

  typedef struct __attribute__((packed)) {
    struct __attribute__((packed)) {
      uint8_t sof;           // 起始字节，固定值为0xA5
      uint16_t data_length;  // 数据帧中data的长度
      uint8_t seq;           // 包序号
      uint8_t crc8;          // 帧头CRC8校验
    } frame_header;          // 帧头
    uint16_t cmd_id;         // 命令码
    union __attribute__((packed)) {
      float angle[6];  // 自定义控制器的数据帧
      uint8_t raw_data[30];
    };
  } Data;  // 自定义控制器数据包

  typedef enum { NUM = 144 } ControllerEvent;

  Param param_;

  bool online_ = false;

  uint32_t last_online_time_ = 0;

  bool StartRecv();

  void Prase();

  void Offline();

  Data data_;

  System::Semaphore packet_recv_ = System::Semaphore(false);

  Message::Event event_;
  System::Thread recv_thread_;
  System::Thread trans_thread_;
};
}  // namespace Device
