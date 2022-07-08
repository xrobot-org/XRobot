#include "FreeRTOS.h"
#include "om.h"
#include "task.h"

#define DECLARE_TOPIC(_name, _arg, _topic_name, _cache) \
  System::Message::Topic<__typeof__(_arg)> _name(_arg, _topic_name, _cache)

#define DECLARE_SUBER(_name, _arg, _topic_name) \
  System::Message::Subscription<__typeof__(_arg)> _name(_topic_name, _arg)

namespace System {
class Message {
 public:
  Message();
  template <typename Data>
  class Topic {
   public:
    Topic(Data& data, const char* name, bool cache) : data_(&data) {
      if (cache) {
        this->topic_ = om_config_topic(NULL, "A", name);
      } else {
        this->topic_ = om_config_topic(NULL, "VA", name);
      }
    }

    Topic(const char* name, bool cache) : data_(NULL) {
      if (cache) {
        this->topic_ = om_config_topic(NULL, "A", name);
      } else {
        this->topic_ = om_config_topic(NULL, "VA", name);
      }
    }

    inline bool Publish() {
      return om_publish(this->topic_, this->data_, sizeof(Data), true, false) ==
             OM_OK;
    }

    inline bool PublishFromISR() {
      return om_publish(this->topic_, this->data_, sizeof(Data), true, true);
    }

    inline om_topic_t* GetHandle() { return this->topic_; }

   private:
    om_topic_t* topic_;
    Data* data_;
  };

  template <typename Data>
  class Subscription {
   public:
    Subscription(const char* name, Data& data) {
      this->handle_ =
          om_subscript(om_find_topic(name, UINT32_MAX), OM_PRASE_VAR(data));
    }

    Subscription(const char* name, om_user_fun_t callback, void* arg) {
      om_topic_t* master = om_find_topic(name, UINT32_MAX);
      this->handle_ = om_config_suber(NULL, "DT", callback, arg, master);
    }

    Subscription(Topic<Data>& topic, om_user_fun_t callback, void* arg) {
      om_topic_t* master = topic.GetHandle();
      this->handle_ = om_config_suber(NULL, "DT", callback, arg, master);
    }

    bool DumpData() { return om_suber_export(this->handle_, false) == OM_OK; }

    bool DumpDataFromISR() {
      return om_suber_export(this->handle_, true) == OM_OK;
    }

   private:
    om_suber_t* handle_;
  };
};
}  // namespace System
