#include "bsp_linux_base.h"

#include <bsp.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

bool bsp_shell_success(int ans) {
  if (-1 == ans) {
    return false;
  } else {
    if (WIFEXITED(ans)) {
      if (0 == WEXITSTATUS(ans)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
}
