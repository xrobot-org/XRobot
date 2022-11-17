#include "term.hpp"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "thread.hpp"

using namespace System;

static System::Thread term_thread_, usb_thread_;

static int TermWrite(const char *data, uint32_t len) {
  bsp_usb_transmit((uint8_t *)data, len);
  return len;
}

Term::Term() {
  bsp_usb_init();

  ms_init(TermWrite);

  auto usb_thread = [](void *arg) {
    (void)arg;
    while (1) {
      bsp_usb_update();
    }
  };

  usb_thread_.Create(usb_thread, (void *)0, "term_thread", 256,
                     System::Thread::Realtime);

  auto term_thread = [](void *arg) {
    (void)arg;
    while (1) {
      while (!bsp_usb_connect()) term_thread_.Sleep(1);

      ms_start();

      while (1) {
        if (bsp_usb_avail()) ms_input(bsp_usb_read_char());
        if (!bsp_usb_connect()) break;
        vTaskDelay(10);
      }
    }
  };

  term_thread_.Create(term_thread, (void *)0, "term_thread", 512,
                      System::Thread::Low);
}
