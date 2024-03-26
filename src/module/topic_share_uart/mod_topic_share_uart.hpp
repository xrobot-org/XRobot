
#include <vector>

#include "bsp_uart.h"
#include "module.hpp"

namespace Module {
template <typename Data>
class TopicShareServerUart {
 public:
  typedef struct {
    const char* topic_name;
    bool block;
    bsp_uart_t uart;
    uint32_t cycle;
  } Param;

  TopicShareServerUart(Param& param)
      : param_(param), topic_(Message::Topic<Data>::Find(param_.topic_name)) {
    XB_ASSERT(topic_.om_topic_);

    auto task_fn = [](TopicShareServerUart* share) {
      share->topic_.PackData(share->data_);
      bsp_uart_transmit(share->param_.uart,
                        reinterpret_cast<uint8_t*>(&share->data_),
                        sizeof(share->data_), share->param_.block);
    };

    System::Timer::Create(task_fn, this, param_.cycle);
  }

  Param param_;
  Message::Topic<Data> topic_;
  typename Message::Topic<Data>::RemoteData data_;
};

class TopicShareClientUart {
 public:
  typedef struct {
    std::vector<const char*> topic_name;
    bool block;
    bsp_uart_t uart;
    uint32_t cycle;
  } Param;

  TopicShareClientUart(Param& param) : param_(param), remote_(128, 10) {
    for (auto name : param_.topic_name) {
      remote_.AddTopic(name);
    }

    auto thread_fn = [](TopicShareClientUart* share) {
      while (true) {
        bsp_uart_receive(share->param_.uart, &share->recv_buff[0],
                         sizeof(share->recv_buff), share->param_.block);
        share->remote_.PraseData(&share->recv_buff[0],
                                 bsp_uart_get_count(share->param_.uart));
        share->thread_.Sleep(share->param_.cycle);
      }
    };

    thread_.Create(thread_fn, this, "topic_share_client", 1024,
                   System::Thread::HIGH);
  };

  Param param_;

  Message::Remote remote_;

  std::array<uint8_t, 128> recv_buff{};

  System::Thread thread_;
};
}  // namespace Module
