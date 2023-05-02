#include <thread.hpp>

#include "bsp_ble_server.h"
#include "bsp_wifi_client.h"
#include "module.hpp"
#include "om.hpp"
#include "om_log.h"

namespace Module {
class BleNetConfig {
 public:
  typedef struct {
    uint8_t net_name[30];
    uint8_t net_password[30];
    bool connected;
  } Data;

  BleNetConfig() : net_info_("net_info") {
    bsp_ble_server_init("XRobot net config");
    bsp_wifi_client_init();
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

        bsp_wifi_connect(reinterpret_cast<char*>(ble->data_.net_name),
                         reinterpret_cast<char*>(ble->data_.net_password));

        uint32_t timeout = 0;

        while (1) {
          if (!bsp_wifi_connected()) {
            timeout += 1000;
            ble->thread_.Sleep(1000);
            OMLOG_NOTICE("Waiting for wifi connection, times %d",
                         timeout / 1000);
            if (timeout > 5000) {
              OMLOG_ERROR("Fail connect to wifi: %s", ble->data_.net_name);
              ble->data_.connected = false;
              break;
            }
          } else {
            OMLOG_PASS("Success connect to wifi: %s", ble->data_.net_name);
            ble->data_.connected = true;
            break;
          }
        }
      }
    };
    thread_.Create(thread_fn, this, "ble_net_config", 2048,
                   System::Thread::MEDIUM);
  }

  uint8_t recv_buff[100];

  Data data_;

  Message::Topic<Data> net_info_;

  System::Thread thread_;
};
}  // namespace Module
