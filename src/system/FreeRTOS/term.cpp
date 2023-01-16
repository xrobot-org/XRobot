#include <term.hpp>
#include <thread.hpp>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

using namespace System;

static System::Thread term_thread, usb_thread;

static int term_write(const char *data, uint32_t len) {
  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(data), len);
  return static_cast<int>(len);
}

Term::Term() {
  bsp_usb_init();

  ms_init(term_write);

  auto usb_thread_fn = [](void *arg) {
    (void)arg;
    while (1) {
      bsp_usb_update();
    }
  };

  usb_thread.Create(usb_thread_fn, static_cast<void *>(0), "term_thread", 256,
                    System::Thread::REALTIME);

  auto term_thread_fn = [](void *arg) {
    (void)arg;
    while (1) {
      while (!bsp_usb_connect()) {
        term_thread.Sleep(1);
      }

      ms_start();

      while (1) {
        if (bsp_usb_avail()) {
          ms_input(bsp_usb_read_char());
        }
        if (!bsp_usb_connect()) {
          break;
        }
        vTaskDelay(10);
      }
    }
  };

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread", 512,
                     System::Thread::LOW);
}
