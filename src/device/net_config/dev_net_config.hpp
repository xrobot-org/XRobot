#include "bsp_time.h"
#include "bsp_wifi_client.h"
#include "device.hpp"

namespace Device {
class NetConfig {
 public:
  typedef struct {
    uint8_t net_name[30];
    uint8_t net_password[30];
    uint32_t time;
    bool connected;
  } Data;

  typedef enum { NOT_CONNECTED, READY_CONNECT, CONNECTING, CONNECTED } Status;

  NetConfig() {
    bsp_wifi_client_init();
    if (Message::Topic<Data>::Find("net_info") == NULL) {
      Message::Topic<Data>("net_info", true);
    }
    sub_ = new Message::Subscriber<Data>("net_info");

    auto thread_fn = [](NetConfig* net) {
      while (true) {
        if (net->sub_->DumpData(net->data_)) {
          if (net->data_.time != net->last_config_time &&
              net->data_.connected) {
            net->status_ = READY_CONNECT;
            net->last_config_time = net->data_.time;
          }
        }

        switch (net->status_) {
          case NOT_CONNECTED:
          case CONNECTED:
            break;
          case READY_CONNECT:
            net->last_connect_time = bsp_time_get_ms();
            net->connect_wait_time = 0;
            OMLOG_NOTICE("Start connect to wifi: %s password %s",
                         net->data_.net_name, net->data_.net_password);
            bsp_wifi_connect(reinterpret_cast<char*>(net->data_.net_name),
                             reinterpret_cast<char*>(net->data_.net_password));
            net->status_ = CONNECTING;
            break;
          case CONNECTING:
            if (bsp_wifi_connected()) {
              net->status_ = CONNECTED;
              OMLOG_PASS("Success connect to wifi: %s", net->data_.net_name);
              break;
            }
            if (net->connect_wait_time > 5000) {
              OMLOG_ERROR("Fail connect to wifi: %s", net->data_.net_name);
              net->status_ = NOT_CONNECTED;
              break;
            }
            net->connect_wait_time += 1000;
            OMLOG_NOTICE("Waiting for wifi connection, times %ds",
                         net->connect_wait_time / 1000);
            break;
        }
        net->thread_.Sleep(1000);
      }
    };
    thread_.Create(thread_fn, this, "net_config", 2048, System::Thread::HIGH);
  }

  Data data_{};

  Status status_ = NOT_CONNECTED;

  uint32_t last_config_time = 0;

  uint32_t last_connect_time = 0;

  uint32_t connect_wait_time = 0;

  Message::Subscriber<Data>* sub_;

  System::Thread thread_;
};
}  // namespace Device
