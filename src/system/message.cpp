#include "message.hpp"

using namespace System;

om_net_t* Message::sys_msg_net_;

Message::Message() {
  om_init();

  sys_msg_net_ = om_create_net("main_msg_net");
}
