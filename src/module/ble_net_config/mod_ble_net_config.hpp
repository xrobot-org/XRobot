#include "bsp_ble_server.h"
#include "bsp_time.h"
#include "dev_net_config.hpp"
#include "module.hpp"

namespace Module {
class BleNetConfig {
 public:
  BleNetConfig() : net_info_("net_info", true) {
    bsp_ble_server_init("XRobot net config");
    data_.connected = 0;
    auto thread_fn = [](BleNetConfig* ble) {
      while (true) {
        ble->net_info_.Publish(ble->data_);

        uint32_t count = bsp_ble_server_avaliable();
        if (!count) {
          ble->thread_.Sleep(100);
          continue;
        }

        bsp_ble_server_receive(&ble->recv_buff[0], count);

        int ans = sscanf(reinterpret_cast<const char*>(ble->recv_buff),
                         "xrobot net_name %s net_password %s end",
                         ble->data_.net_name, ble->data_.net_password);

        if (ans == EOF) {
          continue;
        }

        (void)snprintf(reinterpret_cast<char*>(ble->recv_buff),
                       sizeof(ble->recv_buff),
                       "BLE get net name:%s, net password:%s",
                       ble->data_.net_name, ble->data_.net_password);

        bsp_ble_server_transmit(ble->recv_buff,
                                strnlen(reinterpret_cast<char*>(ble->recv_buff),
                                        sizeof(ble->recv_buff)));

        OMLOG_PASS("%s", ble->recv_buff);

        ble->data_.time = bsp_time_get_ms();
        ble->data_.connected = true;
      }
    };
    thread_.Create(thread_fn, this, "ble_net_config", 2048,
                   System::Thread::MEDIUM);
  }

  uint8_t recv_buff[100]{};

  Device::NetConfig::Data data_{};

  Message::Topic<Device::NetConfig::Data> net_info_;

  System::Thread thread_;
};
}  // namespace Module
