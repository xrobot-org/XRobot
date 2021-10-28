#include "bsp_gpio.h"
#include "bsp_laser.h"

uint8_t Laser_On(void) {
  HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_SET);
  return 0;
}

uint8_t Laser_Off(void) {
  HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_RESET);
  return 0;
}
