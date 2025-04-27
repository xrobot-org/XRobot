#include "dev_spl06_001.hpp"

#include "bsp_i2c.h"

#define PRODUCT_ID_ADDR 0x0D
#define PRODUCT_ID 0x10

#define PRESSURE_REG 0x06
#define TEMPERATURE_REG 0x07
#define MEASUREMENT_CTRL_REG 0x08
#define CONFIGURATION_CTRL_REG 0x09

#define DEVICE_ID 0x77

using namespace Device;

SPL06_001::SPL06_001() {
  while (1) {
    uint8_t tmp =
        bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, PRODUCT_ID_ADDR);
    if (tmp == PRODUCT_ID) {
      break;
    } else {
      OMLOG_ERROR("SPL06_003 PRODUCT_ID read error:%d", tmp);
      System::Thread::Sleep(20);
    }
  }

  uint32_t h = 0;
  uint32_t m = 0;
  uint32_t l = 0;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x10);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x11);
  cali_.c0 = static_cast<int16_t>(h) << 4 | l >> 4;
  cali_.c0 = (cali_.c0 & 0x0800) ? (0xF000 | cali_.c0) : cali_.c0;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x11);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x12);
  cali_.c1 = static_cast<int16_t>(h & 0x0F) << 8 | l;
  cali_.c1 = (cali_.c1 & 0x0800) ? (0xF000 | cali_.c1) : cali_.c1;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x13);
  m = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x14);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x15);
  cali_.c00 = static_cast<int32_t>(h) << 12 | static_cast<int32_t>(m) << 4 |
              static_cast<int32_t>(l) >> 4;
  cali_.c00 = (cali_.c00 & 0x080000) ? (0xFFF00000 | cali_.c00) : cali_.c00;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x15);
  m = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x16);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x17);
  cali_.c10 = static_cast<int32_t>(h) << 16 | static_cast<int32_t>(m) << 8 | l;
  cali_.c10 = (cali_.c10 & 0x080000) ? (0xFFF00000 | cali_.c10) : cali_.c10;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x18);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x19);
  cali_.c01 = static_cast<int16_t>(h) << 8 | l;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1A);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1B);
  cali_.c11 = static_cast<int16_t>(h) << 8 | l;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1C);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1D);
  cali_.c20 = static_cast<int16_t>(h) << 8 | l;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1E);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x1F);
  cali_.c21 = static_cast<int16_t>(h) << 8 | l;
  h = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x20);
  l = bsp_i2c_mem_read_byte(BSP_I2C_SPL, DEVICE_ID, 0x21);
  cali_.c30 = static_cast<int16_t>(h) << 8 | l;

  bsp_i2c_mem_write_byte(BSP_I2C_SPL, DEVICE_ID, PRESSURE_REG, 0x44);
  bsp_i2c_mem_write_byte(BSP_I2C_SPL, DEVICE_ID, TEMPERATURE_REG, 0x44);
  bsp_i2c_mem_write_byte(BSP_I2C_SPL, DEVICE_ID, MEASUREMENT_CTRL_REG, 0x07);
  bsp_i2c_mem_write_byte(BSP_I2C_SPL, DEVICE_ID, CONFIGURATION_CTRL_REG, 0x04);

  auto task_fun = [](SPL06_001* spl) {
    static uint8_t tmp[6];
    bsp_i2c_mem_read(BSP_I2C_SPL, DEVICE_ID, 0x00, tmp, 6, true);

    uint32_t press_raw = (tmp[0] << 16) | (tmp[1] << 8) | tmp[2];
    uint32_t temp_raw = (tmp[3] << 16) | (tmp[4] << 8) | tmp[5];

    if ((press_raw & 0x800000) == 0x800000) {
      press_raw |= 0xFF000000;
    }

    if ((temp_raw & 0x800000) == 0x800000) {
      temp_raw |= 0xFF000000;
    }

    float pressure = static_cast<float>(press_raw) / 253952.0f;
    float temperature = static_cast<float>(temp_raw) / 253952.0f;
    float qua2 = static_cast<float>(spl->cali_.c10) +
                 pressure * (static_cast<float>(spl->cali_.c20) +
                             pressure * static_cast<float>(spl->cali_.c30));
    float qua3 = temperature * pressure *
                 (static_cast<float>(spl->cali_.c11) +
                  pressure * static_cast<float>(spl->cali_.c21));
    spl->data_.pressure = static_cast<float>(spl->cali_.c00) + pressure * qua2 +
                          temperature * static_cast<float>(spl->cali_.c01) +
                          qua3;

    spl->data_.temperature = static_cast<float>(spl->cali_.c0) * 0.5f +
                             static_cast<float>(spl->cali_.c1) * temperature;
  };

  System::Timer::Create(task_fun, this, 250);
}
