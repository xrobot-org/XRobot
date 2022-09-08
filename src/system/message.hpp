#pragma once

#include "FreeRTOS.h"
#include "om.h"
#include "task.h"

#define DECLARE_PUBER(_name, _type, _topic_name, _cache) \
  System::Message::Publisher<__typeof__(_type)> _name =  \
      System::Message::Publisher<__typeof__(_type)>(_topic_name, _cache)

#define DECLARE_TOPIC(_name, _topic_name, _cache) \
  System::Message::Topic _name = System::Message::Topic(_topic_name, _cache)

#define DECLARE_TOPIC_STATIC(_name, _topic_name, _cache)                \
  System::Message::Topic* _name = static_cast<System::Message::Topic*>( \
      System::Memory::Malloc(sizeof(System::Message::Topic)));          \
  new (_name) System::Message::Topic(_topic_name, _cache)

#define DECLARE_SUBER(_name, _buff, _topic_name)         \
  System::Message::Subscriber<__typeof__(_buff)> _name = \
      System::Message::Subscriber<__typeof__(_buff)>(_topic_name, _buff)

#define DECLARE_MESSAGE_FUN(_name) \
  om_user_fun_t _name = [](om_msg_t * msg, void* arg)
#define GetMessage(_type, _name) _type* _name = static_cast<_type*>(msg->buff)
#define GetARG(_type, _name) _type* _name = static_cast<_type*>(arg)
#define MESSAGE_FUN_PASSED() return OM_OK
#define MESSAGE_FUN_FAILED() return OM_ERROR

#define MESSAGE_REGISTER_FILTER(_topic, _callback, _arg)            \
  (void)(((__typeof__(_topic)*)0) == ((System::Message::Topic*)0)); \
  om_config_topic(_topic.GetHandle(), "F", _callback, _arg)

#define MESSAGE_REGISTER_CALLBACK(_topic, _callback, _arg)          \
  (void)(((__typeof__(_topic)*)0) == ((System::Message::Topic*)0)); \
  om_config_topic(_topic.GetHandle(), "D", _callback, _arg)

namespace System {
class Message {
 public:
  Message();
  class Topic {
   public:
    Topic(const char* name, bool cache) {
      if (cache) {
        this->topic_ = om_config_topic(NULL, "A", name);
      } else {
        this->topic_ = om_config_topic(NULL, "VA", name);
      }
    }

    bool Link(Topic& source) {
      return om_core_link(source.GetHandle(), this->GetHandle()) == OM_OK;
    }

    bool Link(const char* source_name) {
      return om_core_link(om_find_topic(source_name, UINT32_MAX),
                          this->GetHandle()) == OM_OK;
    }

    inline om_topic_t* GetHandle() { return this->topic_; }

    om_topic_t* topic_;
  };

  template <typename Data>
  class Publisher : public Topic {
   public:
    Publisher(const char* name, bool cache) : Topic(name, cache) {}

    inline bool Publish() {
      return om_publish(this->topic_, &(this->data_), sizeof(Data), true,
                        false) == OM_OK;
    }

    inline bool PublishFromISR() {
      return om_publish(this->topic_, &(this->data_), sizeof(Data), true, true);
    }

    Data data_;
  };

  template <typename Data>
  class Subscriber {
   public:
    Subscriber(const char* name, Data& data) {
      this->handle_ =
          om_subscript(om_find_topic(name, UINT32_MAX), OM_PRASE_VAR(data));
    }

    bool DumpData() { return om_suber_export(this->handle_, false) == OM_OK; }

    bool DumpDataFromISR() {
      return om_suber_export(this->handle_, true) == OM_OK;
    }

    om_suber_t* handle_;
  };

  class Event {
   public:
    typedef enum {
      EventStart = OM_EVENT_START,
      EventProgress = OM_EVENT_PROGRESS,
      EventEnd = OM_EVENT_END
    } Status;

    Event(const char* name) { this->group_ = om_event_create_group(name); }

    Event(om_event_group_t group) : group_(group) {}

    static om_event_group_t FindEvent(const char* name) {
      return om_event_find_group(name, UINT32_MAX);
    }

    bool Register(uint32_t event, Status status,
                  void (*callback)(uint32_t event, void* arg), void* arg) {
      return om_event_register(this->group_, event, (om_event_status_t)status,
                               callback, arg);
    }

    bool Active(uint32_t event) {
      return om_event_active(this->group_, event, true, false);
    }

    bool ActiveFromISR(uint32_t event) {
      return om_event_active(this->group_, event, true, true);
    }

    om_event_group_t group_;
  };
};
}  // namespace System
