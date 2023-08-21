#include <atomic>
#include <cstring>
#include <database.hpp>
#include <term.hpp>

#include "ms.h"

using namespace System;

static uint8_t sn_buff[33];

static ms_item_t sn_tools;

Database::Database() {
  bsp_flash_init();

  // std::atomic_thread_fence();

  auto sn_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    OM_UNUSED(item);

    if (argc == 1) {
      printf("-show        show SN code.\r\n");

      printf("-set [code]  set  SN code.\r\n");

    } else if (argc == 2) {
      if (strcmp("show", argv[1]) == 0) {
        memset(sn_buff, 0, sizeof(sn_buff));
        bsp_flash_get_blog("SN", sn_buff, sizeof(sn_buff) - 1);
        sn_buff[32] = '\0';
        printf("SN:%s\r\n", sn_buff);

      } else {
        printf("Error command.\r\n");
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
          printf("SN:%s\r\n", sn_buff);

        } else {
          printf("Error sn code format: %s\r\n", argv[2]);
        }
      } else {
        printf("Error sn code format: %s\r\n", argv[2]);
      }
    }

    return 0;
  };

  ms_file_init(&sn_tools, "sn_tools", sn_cmd_fn, sn_buff, sizeof(sn_buff),
               false);
  ms_cmd_add(&sn_tools);
}
