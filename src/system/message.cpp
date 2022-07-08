#include "message.hpp"

using namespace System;

Message::Message() {
  om_init();

  auto sync_thread = [](void* arg) {
    OM_UNUSED(arg);
    while (1) {
      om_sync(false);
      vTaskDelay(1);
    }
  };

  xTaskCreate(sync_thread, "message", 128, NULL, 5, NULL);
}
