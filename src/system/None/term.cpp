#include <term.hpp>
#include <timer.hpp>

#include "bsp_usb.h"

using namespace System;

static bool connected = false;

static om_status_t print_log(om_msg_t *msg, void *arg) {
  (void)arg;

  if (!bsp_usb_connect()) {
    return OM_ERROR_NOT_INIT;
  }

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  ms_printf_insert("%-.4f %s", bsp_time_get(), log->data);

  return OM_OK;
}

static int term_write(const char *data, size_t len) {
  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(data), len);
  return static_cast<int>(len);
}

Term::Term() {
  bsp_usb_init();

  ms_init(term_write);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto usb_thread_fn = [](void *arg) {
    (void)arg;
    bsp_usb_update();
  };

  System::Timer::Create(usb_thread_fn, static_cast<void *>(0), 10);

  auto term_thread_fn = [](void *arg) {
    (void)arg;
    if (!bsp_usb_connect()) {
      connected = false;
      return;
    } else {
      if (!connected) {
        connected = true;
        ms_start();
      }
    }

    if (bsp_usb_avail()) {
      ms_input(bsp_usb_read_char());
    }
  };

  System::Timer::Create(term_thread_fn, static_cast<void *>(0), 10);
}
