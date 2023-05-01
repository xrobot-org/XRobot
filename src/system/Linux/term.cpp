
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <term.hpp>
#include <thread.hpp>

#include "bsp_time.h"
#include "bsp_udp_server.h"
#include "ms.h"
#include "om.hpp"

using namespace System;

static System::Thread term_thread, term_udp_thread;

static bsp_udp_server_t term_udp_server;

static ms_item_t log_control;

static bool log_enable = false;

static int kbhit(void) {
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

static int log_ctrl_fn(ms_item_t *item, int argc, char **argv) {
  MS_UNUSED(item);
  if (argc == 1) {
    printf("on开启/off关闭\r\n");
  } else if (argc == 2) {
    if (strcmp(argv[1], "on") == 0) {
      ms_clear();
      log_enable = true;
    } else if (strcmp(argv[1], "off") == 0) {
      log_enable = false;
    } else {
      printf("命令错误。\r\n");
    }
  } else {
    printf("命令错误。\r\n");
  }

  return 0;
}

int show_fun(const char *data, uint32_t len) {
  if (log_enable) {
    return OM_OK;
  }

  while (len--) {
    putchar(*data++);
  }

  return 0;
}

static om_status_t print_log(om_msg_t *msg, void *arg) {
  (void)arg;

  static char time_print_buff[20];

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  snprintf(time_print_buff, sizeof(time_print_buff), "%-.4f ", bsp_time_get());

#ifdef TERM_LOG_UDP_SERVER
  bsp_udp_server_transmit(&term_udp_server,
                          reinterpret_cast<const uint8_t *>(time_print_buff),
                          strlen(time_print_buff));

  bsp_udp_server_transmit(&term_udp_server,
                          reinterpret_cast<const uint8_t *>(log->data),
                          strlen(log->data));
#endif

  if (!log_enable) {
    return OM_OK;
  }

  printf("%s%s", time_print_buff, log->data);

  return OM_OK;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

  ms_file_init(&log_control, "log", log_ctrl_fn, NULL, NULL);
  ms_cmd_add(&log_control);

  auto term_udp_thread_fn = [](void *arg) {
    (void)arg;
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
  (void)term_udp_server;
  (void)term_udp_thread_fn;
  (void)term_udp_thread;
#endif

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto term_thread_fn = [](void *arg) {
    (void)arg;

    ms_start();

    while (1) {
      if (kbhit()) {
        ms_input(static_cast<char>(getchar()));
      } else {
        System::Thread::Sleep(10);
      }
    }
  };

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread", 512,
                     System::Thread::LOW);
}
