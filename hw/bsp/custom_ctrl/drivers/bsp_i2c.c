#include "bsp_i2c.h"

#include "bsp_def.h"
#include "bsp_time.h"
#include "main.h"

typedef struct {
  GPIO_TypeDef *scl_gpio;
  uint16_t scl_pin;
  GPIO_TypeDef *sda_gpio;
  uint16_t sda_pin;
} bsp_i2c_map_t;

static const bsp_i2c_map_t BSP_I2C_MAP[] = {
    {I2C1_SCL_GPIO_Port, I2C1_SCL_Pin, I2C1_SDA_GPIO_Port, I2C1_SDA_Pin},
    {I2C2_SCL_GPIO_Port, I2C2_SCL_Pin, I2C2_SDA_GPIO_Port, I2C2_SDA_Pin},
    {I2C3_SCL_GPIO_Port, I2C3_SCL_Pin, I2C3_SDA_GPIO_Port, I2C3_SDA_Pin},
    {I2C4_SCL_GPIO_Port, I2C4_SCL_Pin, I2C4_SDA_GPIO_Port, I2C4_SDA_Pin},
    {I2C5_SCL_GPIO_Port, I2C5_SCL_Pin, I2C5_SDA_GPIO_Port, I2C5_SDA_Pin},
    {I2C6_SCL_GPIO_Port, I2C6_SCL_Pin, I2C6_SDA_GPIO_Port, I2C6_SDA_Pin},
};

#define I2C_SCL_WritePin(_x, _i2c)                                         \
  HAL_GPIO_WritePin(BSP_I2C_MAP[_i2c].scl_gpio, BSP_I2C_MAP[_i2c].scl_pin, \
                    ((_x) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA_WritePin(_x, _i2c)                                         \
  HAL_GPIO_WritePin(BSP_I2C_MAP[_i2c].sda_gpio, BSP_I2C_MAP[_i2c].sda_pin, \
                    ((_x) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA_ReadPin(_i2c) \
  HAL_GPIO_ReadPin(BSP_I2C_MAP[_i2c].sda_gpio, BSP_I2C_MAP[_i2c].sda_pin)

static void Delay_us(uint32_t t) {
  uint64_t time = bsp_time_get();
  while (bsp_time_get() < time + t) {
  }
}

void I2C_Start(bsp_i2c_t i2c) {
  I2C_SDA_WritePin(1, i2c);
  I2C_SCL_WritePin(1, i2c);
  Delay_us(10);
  I2C_SDA_WritePin(0, i2c);
  Delay_us(10);
}

void I2C_Stop(bsp_i2c_t i2c) {
  I2C_SCL_WritePin(1, i2c);
  I2C_SDA_WritePin(0, i2c);
  Delay_us(10);
  I2C_SDA_WritePin(1, i2c);
  Delay_us(10);
}

HAL_StatusTypeDef I2C_WaitAck(bsp_i2c_t i2c) {
  uint8_t ErrorTime = 20;
  I2C_SDA_WritePin(1, i2c);
  I2C_SCL_WritePin(1, i2c);
  while (I2C_SDA_ReadPin(i2c) && (--ErrorTime)) {
    Delay_us(1);
  }
  if (!ErrorTime) return HAL_ERROR;

  Delay_us(10);
  I2C_SCL_WritePin(0, i2c);
  Delay_us(10);
  return HAL_OK;
}

void I2C_SendByte(uint8_t byte, bsp_i2c_t i2c) {
  I2C_SCL_WritePin(0, i2c);
  for (uint8_t i = 0; i < 8; i++) {
    I2C_SDA_WritePin(byte & 0x80, i2c);  // 高位先写
    byte <<= 1;
    Delay_us(10);
    I2C_SCL_WritePin(1, i2c);
    Delay_us(10);
    I2C_SCL_WritePin(0, i2c);
    Delay_us(10);
  }
}

uint8_t I2C_ReceiveByte(bsp_i2c_t i2c) {
  uint8_t byte = 0;
  I2C_SDA_WritePin(1, i2c);
  for (uint8_t i = 0; i < 8; i++) {
    byte <<= 1;
    Delay_us(10);
    I2C_SCL_WritePin(1, i2c);
    if (I2C_SDA_ReadPin(i2c)) {
      byte |= 0x01;
    }
    Delay_us(10);
    I2C_SCL_WritePin(0, i2c);
    Delay_us(10);
  }
  return byte;
}

void I2C_SendAck(bsp_i2c_t i2c) {
  I2C_SDA_WritePin(0, i2c);
  Delay_us(10);
  I2C_SCL_WritePin(1, i2c);
  Delay_us(10);
  I2C_SCL_WritePin(0, i2c);
  Delay_us(10);
}

void I2C_SendNack(bsp_i2c_t i2c) {
  I2C_SDA_WritePin(1, i2c);
  Delay_us(10);
  I2C_SCL_WritePin(1, i2c);
  Delay_us(10);
  I2C_SCL_WritePin(0, i2c);
  Delay_us(10);
}

bsp_status_t bsp_i2c_transmit(bsp_i2c_t i2c, uint8_t addr, uint8_t *data,
                              size_t size, bool block) {
  UNUSED(i2c);
  UNUSED(block);
  I2C_Start(i2c);
  I2C_SendByte(addr << 1, i2c);
  if (I2C_WaitAck(i2c) == HAL_ERROR) {
    I2C_Stop(i2c);
    return BSP_ERR;
  }
  for (size_t i = 0; i < size; i++) {
    I2C_SendByte(data[i], i2c);
    if (I2C_WaitAck(i2c) == HAL_ERROR) {
      I2C_Stop(i2c);
      return BSP_ERR;
    }
  }
  I2C_Stop(i2c);
  return BSP_OK;
}

bsp_status_t bsp_i2c_receive(bsp_i2c_t i2c, uint8_t addr, uint8_t *buff,
                             size_t size, bool block) {
  UNUSED(i2c);
  UNUSED(block);
  I2C_Start(i2c);
  I2C_SendByte((addr << 1) | 0x01, i2c);
  if (I2C_WaitAck(i2c) == HAL_ERROR) {
    I2C_Stop(i2c);
    return BSP_ERR;
  }
  for (size_t i = 0; i < size; i++) {
    buff[i] = I2C_ReceiveByte(i2c);
    if (i < size - 1)
      I2C_SendAck(i2c);
    else
      I2C_SendNack(i2c);
  }
  I2C_Stop(i2c);
  return BSP_OK;
}

uint8_t bsp_i2c_mem_read_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg) {
  UNUSED(i2c);
  uint8_t data = 0;
  bsp_i2c_transmit(i2c, addr, &reg, 1, true);
  bsp_i2c_receive(i2c, addr, &data, 1, true);
  return data;
}

bsp_status_t bsp_i2c_mem_write_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                                    uint8_t data) {
  uint8_t buffer[2] = {reg, data};
  return bsp_i2c_transmit(i2c, addr, buffer, 2, true);
}

bsp_status_t bsp_i2c_mem_read(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                              uint8_t *data, size_t size, bool block) {
  UNUSED(i2c);
  UNUSED(block);
  bsp_i2c_transmit(i2c, addr, &reg, 1, true);
  return bsp_i2c_receive(i2c, addr, data, size, true);
}

bsp_status_t bsp_i2c_mem_write(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                               uint8_t *buff, size_t size, bool block) {
  UNUSED(i2c);
  UNUSED(block);
  uint8_t *buffer = malloc(size + 1);
  buffer[0] = reg;
  memcpy(buffer + 1, buff, size);
  bsp_status_t status = bsp_i2c_transmit(i2c, addr, buffer, size + 1, true);
  free(buffer);
  return status;
}
