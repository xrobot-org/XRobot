#include "database.hpp"

#include <string.h>

#include "ms.h"
#include "term.hpp"

using namespace System;

static uint8_t sn_buff[33];

static ms_item_t sn_tools;

static int sn_cmd_fn(int argc, char *agrv[]) {
  if (argc == 1) {
    ms_printf("-show        show SN code.");
    ms_enter();
    ms_printf("-set [code]  set  SN code.");
    ms_enter();
  } else if (argc == 2) {
    if (strcmp("show", agrv[1]) == 0) {
      Database::Get("SN", sn_buff, sizeof(sn_buff) - 1);
      sn_buff[32] = '\0';
      ms_printf("SN:%s", sn_buff);
      ms_enter();
    } else {
      ms_printf("Error command.", sn_buff);
      ms_enter();
    }
  } else if (argc == 3) {
    if (strcmp("set", agrv[1]) == 0 && strlen(agrv[2]) == 32) {
      bool check_ok = true;

      for (uint8_t i = 0; i < 32; i++) {
        if (isalnum(agrv[2][i])) {
          sn_buff[i] = agrv[2][i];
        } else {
          check_ok = false;
          break;
        }
      }

      if (check_ok) {
        Database::Set("SN", sn_buff, sizeof(sn_buff) - 1);
        sn_buff[32] = '\0';
        ms_printf("SN:%s", sn_buff);
        ms_enter();
      } else {
        ms_printf("SN code format error.", sn_buff);
        ms_enter();
      }
    } else {
      ms_printf("Error command.", sn_buff);
      ms_enter();
    }
  }

  return 0;
}

Database::Database() {
  easyflash_init();
  ms_file_init(&sn_tools, "sn_tools", sn_cmd_fn, NULL, NULL);
  ms_cmd_add(&sn_tools);
}
