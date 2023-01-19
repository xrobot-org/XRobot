#include <cstring>
#include <database.hpp>
#include <term.hpp>

#include "ms.h"

using namespace System;

static uint8_t sn_buff[33];

static ms_item_t sn_tools;

Database::Database() {
  bsp_flash_init();

  auto sn_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    MS_UNUSED(item);

    if (argc == 1) {
      ms_printf("-show        show SN code.");
      ms_enter();
      ms_printf("-set [code]  set  SN code.");
      ms_enter();
    } else if (argc == 2) {
      if (strcmp("show", argv[1]) == 0) {
        memset(sn_buff, 0, sizeof(sn_buff));
        bsp_flash_get_blog("SN", sn_buff, sizeof(sn_buff) - 1);
        sn_buff[32] = '\0';
        ms_printf("SN:%s", sn_buff);
        ms_enter();
      } else {
        ms_printf("Error command.");
        ms_enter();
      }
    } else if (argc == 3) {
      if (strcmp("set", argv[1]) == 0 && strlen(argv[2]) == 32) {
        bool check_ok = true;

        for (uint8_t i = 0; i < 32; i++) {
          if (isalnum(argv[2][i])) {
            sn_buff[i] = argv[2][i];
          } else {
            check_ok = false;
            break;
          }
        }

        if (check_ok) {
          bsp_flash_set_blog("SN", sn_buff, sizeof(sn_buff) - 1);
          sn_buff[32] = '\0';
          ms_printf("SN:%s", sn_buff);
          ms_enter();
        } else {
          ms_printf("Error sn code format: %s", argv[2]);
          ms_enter();
        }
      } else {
        ms_printf("Error sn code format: %s", argv[2]);
        ms_enter();
      }
    }

    return 0;
  };

  ms_file_init(&sn_tools, "sn_tools", sn_cmd_fn, NULL, NULL);
  ms_cmd_add(&sn_tools);
}
