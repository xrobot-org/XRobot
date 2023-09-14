#include <poll.h>
#include <stdint.h>

#include <array>
#include <database.hpp>
#include <term.hpp>

#include "ms.h"

using namespace System;

static ms_item_t sn_tools;

std::string Database::path_(std::string(getenv("HOME")) + "/.rm_database/");

Database::Key<std::array<uint8_t, 32>> *sn;

Database::Database() {
  auto sn_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    OM_UNUSED(item);

    if (argc == 1) {
      printf("-show        show SN code.\r\n");

      printf("-set [code]  set  SN code.\r\n");

    } else if (argc == 2) {
      if (strcmp("show", argv[1]) == 0) {
        sn->Get();
        printf("SN\r\n:%.32s", sn->data_);

      } else {
        printf("Error command.\r\n");
      }
    } else if (argc == 3) {
      if (strcmp("set", argv[1]) == 0 && strlen(argv[2]) == 32) {
        bool check_ok = true;

        for (uint8_t i = 0; i < 32; i++) {
          if (isalnum(argv[2][i])) {
            sn->data_[i] = argv[2][i];
          } else {
            check_ok = false;
            sn->Get();
            break;
          }
        }

        if (check_ok) {
          sn->Set();
          printf("SN:%.32s\r\n", sn->data_);

        } else {
          printf("Error sn code format: %s\r\n", argv[2]);
        }
      } else {
        printf("Error sn code format: %s\r\n", argv[2]);
      }
    }

    return 0;
  };

  poll(NULL, 0, 1);

  mkdir(path_.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

  sn = new Database::Key<std::array<uint8_t, 32>>("SN");

  ms_file_init(&sn_tools, "sn_tools", sn_cmd_fn, &(sn->data_),
               sizeof(sn->data_), false);
  ms_cmd_add(&sn_tools);
}
