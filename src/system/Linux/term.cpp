
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <cstddef>
#include <term.hpp>
#include <thread.hpp>

#include "bsp_sys.h"
#include "bsp_time.h"
#include "bsp_udp_server.h"
#include "ms.h"
#include "om.hpp"

using namespace System;

static System::Thread term_thread, term_udp_thread;

static bsp_udp_server_t term_udp_server;

static ms_item_t power_ctrl;

static int kbhit() {
  struct termios oldt, newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

int show_fun(const char *data, size_t len) {
  while (len--) {
    putchar(*data++);
  }

  return 0;
}

static om_status_t print_log(om_msg_t *msg, void *arg) {
  XB_UNUSED(arg);

  static char time_print_buff[20];

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  (void)snprintf(time_print_buff, sizeof(time_print_buff), "%-.4f ",
                 static_cast<float>(bsp_time_get()) / 1000000.0f);

#ifdef TERM_LOG_UDP_SERVER
  bsp_udp_server_transmit(&term_udp_server,
                          reinterpret_cast<const uint8_t *>(time_print_buff),
                          strlen(time_print_buff));

  bsp_udp_server_transmit(&term_udp_server,
                          reinterpret_cast<const uint8_t *>(log->data),
                          strlen(log->data));
#endif

  ms_printf_insert("%s%s", time_print_buff, log->data);

  return OM_OK;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

  auto term_udp_thread_fn = [](void *arg) {
    XB_UNUSED(arg);
    bsp_udp_server_start(&term_udp_server);
    while (true) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

#ifdef TERM_LOG_UDP_SERVER
  bsp_udp_server_init(&term_udp_server, TERM_LOG_UDP_SERVER_PORT);

  term_udp_thread.Create(term_udp_thread_fn, static_cast<void *>(0),
                         "term_udp_thread", 512, System::Thread::HIGH);
#else
  XB_UNUSED(term_udp_server);
  XB_UNUSED(term_udp_thread_fn);
  XB_UNUSED(term_udp_thread);
#endif

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto term_thread_fn = [](void *arg) {
    XB_UNUSED(arg);

    ms_start();

    while (1) {
      if (kbhit()) {
        ms_input(static_cast<char>(getchar()));
      } else {
        System::Thread::Sleep(10);
      }
    }
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

  ms_file_init(&power_ctrl, "power", pwr_cmd_fn, NULL, 0, false);
  ms_cmd_add(&power_ctrl);

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread", 512,
                     System::Thread::LOW);
}
