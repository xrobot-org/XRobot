#include "bsp.h"
#include "thd.h"

int main(void) {
  /* Init Clock and Peripheral */
  bsp_init();

  /* Init task */
  thd_init();

  /* Start scheduler */
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1) {
  }
}
