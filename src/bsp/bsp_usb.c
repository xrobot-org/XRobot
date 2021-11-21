#include "bsp_usb.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp_delay.h"
#include "comp_utils.h"
/* Private define -----------------s------------------------------------------*/
#define APP_RX_DATA_SIZE 1024
#define APP_TX_DATA_SIZE 1024
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */

extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

static int8_t BSP_USB_Transmit(uint8_t *buffer, uint16_t len) {
  uint8_t retry = 0;
  // do {
  //   if (CDC_Transmit_FS(buffer, len) != USBD_OK) {
  //     retry++;
  //     BSP_Delay(10);
  //   } else {
  //     break;
  //   }
  // } while (retry < 3);
  return BSP_OK;
}

/* Exported functions ------------------------------------------------------- */
int8_t BSP_USB_StartReceive(void) {
  // CDC_StartReceive();
  return BSP_OK;
}

char BSP_USB_ReadChar(void) {
  // return UserRxBufferFS[0];
}

int8_t BSP_USB_Printf(const char *fmt, ...) {
  /*va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf((char *)UserTxBufferFS, APP_TX_DATA_SIZE - 1, fmt, ap);
  va_end(ap);

  if (len > 0) {
    BSP_USB_Transmit(UserTxBufferFS, (uint16_t)(len));
    return BSP_OK;
  } else {
    return BSP_ERR_NULL;
  }*/
}
