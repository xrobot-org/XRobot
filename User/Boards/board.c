#include "board.h"
#include "main.h"
#include "pin.h"

#include <stdio.h>
#include <stdarg.h>

#include "cmsis_os.h"

#include "rng.h"
#include "usbd_cdc_if.h"

static uint8_t usb_buf[256];

int Board_Delay(uint32_t ms) {
	if (osKernelGetState() == osKernelRunning)
		osDelay(ms);
	else
		HAL_Delay(ms);
	
	return 0;
}

uint32_t Board_GetRandomNum(void){
	static uint32_t rng;
	HAL_RNG_GenerateRandomNumber(&hrng, &rng);
	return rng;	
}

int32_t Board_GetRandomRange(int min, int max) {
	static int32_t random;
	random = (Board_GetRandomNum() % (max - min + 1)) + min;
	return random;
}

int Board_USBPrint(const char *fmt,...) {
	static va_list ap;
	uint16_t len = 0;

	va_start(ap, fmt);
	len = vsprintf((char *)usb_buf, fmt, ap);
	va_end(ap);

	CDC_Transmit_FS(usb_buf, len);
	
	return 0;
}
