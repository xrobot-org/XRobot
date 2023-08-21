
#include <cmath>
#include <cstdlib>
#include <term.hpp>
#include <thread.hpp>

#include "bsp_sys.h"
#include "bsp_time.h"
#include "bsp_usb.h"
#include "ms.h"
#include "om.hpp"
#include "om_def.h"

using namespace System;

static System::Thread term_thread, usb_thread;

static ms_item_t task_info, power_ctrl, date;

#ifdef MCU_DEBUG_BUILD
static char task_print_buff[1024];
#endif

extern ms_t ms;

static om_status_t print_log(om_msg_t *msg, void *arg) {
  XB_UNUSED(arg);

  if (!bsp_usb_connect()) {
    return OM_ERROR_NOT_INIT;
  }

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  ms_printf_insert("%-.4f %s", static_cast<float>(bsp_time_get()) / 1000000.0f,
                   log->data);

  return OM_OK;
}

static int term_write(const char *data, size_t len) {
  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(data), len);
  return static_cast<int>(len);
}

int printf(const char *format, ...) {
  va_list v_arg_list;
  va_start(v_arg_list, format);
  XB_UNUSED(vsnprintf(ms.buff.write_buff, sizeof(ms.buff.write_buff), format,
                      v_arg_list));
  va_end(v_arg_list);

  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(ms.buff.write_buff),
                   strlen(ms.buff.write_buff));

  return 0;
}

Term::Term() {
  bsp_usb_init();

  ms_init(term_write);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

#ifdef MCU_DEBUG_BUILD

  auto task_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    XB_UNUSED(item);
    XB_UNUSED(argc);
    XB_UNUSED(argv);

    vTaskList(task_print_buff);
    printf("Name            State   Pri     Stack   Num\r\n");
    bsp_usb_transmit(reinterpret_cast<const uint8_t *>(task_print_buff),
                     strnlen(task_print_buff, sizeof(task_print_buff)));

    return 0;
  };

  auto pwr_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    XB_UNUSED(item);

    if (argc == 1) {
      printf("Please add option:shutdown reboot sleep or stop.\r\n");
    } else if (argc == 2) {
      if (strcmp(argv[1], "sleep") == 0) {
        bsp_sys_sleep();
      } else if (strcmp(argv[1], "stop") == 0) {
        bsp_sys_stop();
      } else if (strcmp(argv[1], "shutdown") == 0) {
        bsp_sys_shutdown();
      } else if (strcmp(argv[1], "reboot") == 0) {
        bsp_sys_reset();
      }
    }

    return 0;
  };

  auto date_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    XB_UNUSED(item);
    XB_UNUSED(argc);
    XB_UNUSED(argv);

    uint32_t time = bsp_time_get_ms();

    printf("%d days %d hours %d minutes %.3f seconds\r\n",
           time / 24 / 3600 / 1000, time / 1000 % (24 * 3600) / 3600,
           time / 1000 % 3600 / 60,
           fmodf(static_cast<float>(time), 60 * 1000) / 1000.0);

    return 0;
  };

  ms_file_init(&task_info, "task_info", task_cmd_fn, NULL, 0, false);
  ms_cmd_add(&task_info);

  ms_file_init(&power_ctrl, "power", pwr_cmd_fn, NULL, 0, false);
  ms_cmd_add(&power_ctrl);

  ms_file_init(&date, "date", date_cmd_fn, NULL, 0, false);
  ms_cmd_add(&date);

#endif

  auto usb_thread_fn = [](void *arg) {
    XB_UNUSED(arg);
    while (1) {
      bsp_usb_update();
      vTaskDelay(10);
    }
  };

  usb_thread.Create(usb_thread_fn, static_cast<void *>(0), "usb_thread",
                    FREERTOS_USB_TASK_STACK_DEPTH, System::Thread::HIGH);

  auto term_thread_fn = [](void *arg) {
    XB_UNUSED(arg);
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

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread",
                     FREERTOS_TERM_TASK_STACK_DEPTH, System::Thread::REALTIME);
}
