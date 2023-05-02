
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
    ASSERT(topic_.om_topic_);

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
}  // namespace Module
