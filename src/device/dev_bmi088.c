/*
        BMI088 陀螺仪+加速度计传感器。

*/

#include "dev_bmi088.h"

#include <stdbool.h>
#include <string.h>

#include "bsp_delay.h"
#include "bsp_gpio.h"
#include "bsp_spi.h"
#include "comp_utils.h"

#define BMI088_REG_ACCL_CHIP_ID (0x00)
#define BMI088_REG_ACCL_ERR (0x02)
#define BMI088_REG_ACCL_STATUS (0x03)
#define BMI088_REG_ACCL_X_LSB (0x12)
#define BMI088_REG_ACCL_X_MSB (0x13)
#define BMI088_REG_ACCL_Y_LSB (0x14)
#define BMI088_REG_ACCL_Y_MSB (0x15)
#define BMI088_REG_ACCL_Z_LSB (0x16)
#define BMI088_REG_ACCL_Z_MSB (0x17)
#define BMI088_REG_ACCL_SENSORTIME_0 (0x18)
#define BMI088_REG_ACCL_SENSORTIME_1 (0x19)
#define BMI088_REG_ACCL_SENSORTIME_2 (0x1A)
#define BMI088_REG_ACCL_INT_STAT_1 (0x1D)
#define BMI088_REG_ACCL_TEMP_MSB (0x22)
#define BMI088_REG_ACCL_TEMP_LSB (0x23)
#define BMI088_REG_ACCL_CONF (0x40)
#define BMI088_REG_ACCL_RANGE (0x41)
#define BMI088_REG_ACCL_INT1_IO_CONF (0x53)
#define BMI088_REG_ACCL_INT2_IO_CONF (0x54)
#define BMI088_REG_ACCL_INT1_INT2_MAP_DATA (0x58)
#define BMI088_REG_ACCL_SELF_TEST (0x6D)
#define BMI088_REG_ACCL_PWR_CONF (0x7C)
#define BMI088_REG_ACCL_PWR_CTRL (0x7D)
#define BMI088_REG_ACCL_SOFTRESET (0x7E)

#define BMI088_REG_GYRO_CHIP_ID (0x00)
#define BMI088_REG_GYRO_X_LSB (0x02)
#define BMI088_REG_GYRO_X_MSB (0x03)
#define BMI088_REG_GYRO_Y_LSB (0x04)
#define BMI088_REG_GYRO_Y_MSB (0x05)
#define BMI088_REG_GYRO_Z_LSB (0x06)
#define BMI088_REG_GYRO_Z_MSB (0x07)
#define BMI088_REG_GYRO_INT_STAT_1 (0x0A)
#define BMI088_REG_GYRO_RANGE (0x0F)
#define BMI088_REG_GYRO_BANDWIDTH (0x10)
#define BMI088_REG_GYRO_LPM1 (0x11)
#define BMI088_REG_GYRO_SOFTRESET (0x14)
#define BMI088_REG_GYRO_INT_CTRL (0x15)
#define BMI088_REG_GYRO_INT3_INT4_IO_CONF (0x16)
#define BMI088_REG_GYRO_INT3_INT4_IO_MAP (0x18)
#define BMI088_REG_GYRO_SELF_TEST (0x3C)

#define BMI088_CHIP_ID_ACCL (0x1E)
#define BMI088_CHIP_ID_GYRO (0x0F)

#define BMI088_ACCL_RX_BUFF_LEN (19)
#define BMI088_GYRO_RX_BUFF_LEN (6)

#define BMI088_ACCL_NSS_SET() bsp_gpio_write_pin(BSP_GPIO_IMU_ACCL_CS, true)
#define BMI088_ACCL_NSS_RESET() bsp_gpio_write_pin(BSP_GPIO_IMU_ACCL_CS, false)

#define BMI088_GYRO_NSS_SET() bsp_gpio_write_pin(BSP_GPIO_IMU_GYRO_CS, true)
#define BMI088_GYRO_NSS_RESET() bsp_gpio_write_pin(BSP_GPIO_IMU_GYRO_CS, false)
typedef enum {
  BMI_ACCL,
  BMI_GYRO,
} BMI_Device_t;

static uint8_t tx_rx_buf[2];
static uint8_t dma_buf[BMI088_ACCL_RX_BUFF_LEN + BMI088_GYRO_RX_BUFF_LEN];

static bool inited = false;

static void bmi_write_single(BMI_Device_t dv, uint8_t reg, uint8_t data) {
  tx_rx_buf[0] = (reg & 0x7f);
  tx_rx_buf[1] = data;

  bsp_delay(1);
  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }

  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 2u, true);

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_SET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_SET();
      break;
  }
}

static uint8_t bmi_read_single(BMI_Device_t dv, uint8_t reg) {
  bsp_delay(1);
  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }
  tx_rx_buf[0] = (uint8_t)(reg | 0x80);
  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 1u, true);
  bsp_spi_receive(BSP_SPI_IMU, tx_rx_buf, 2u, true);

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_SET();
      return tx_rx_buf[1];

    case BMI_GYRO:
      BMI088_GYRO_NSS_SET();
      return tx_rx_buf[0];
  }
  return DEVICE_OK;
}

static void bmi_read(BMI_Device_t dv, uint8_t reg, uint8_t *data, uint8_t len) {
  ASSERT(data);

  switch (dv) {
    case BMI_ACCL:
      BMI088_ACCL_NSS_RESET();
      break;

    case BMI_GYRO:
      BMI088_GYRO_NSS_RESET();
      break;
  }
  tx_rx_buf[0] = (uint8_t)(reg | 0x80);
  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 1u, true);
  bsp_spi_receive(BSP_SPI_IMU, data, len, false);
}

static void bmi088_rx_cplt_callback(void *arg) {
  bmi088_t *bmi088 = arg;
  BaseType_t switch_required;
  if (!bsp_gpio_read_pin(BSP_GPIO_IMU_ACCL_CS)) {
    BMI088_ACCL_NSS_SET();
    xSemaphoreGiveFromISR(bmi088->sem.accl_raw, &switch_required);
  }
  if (!bsp_gpio_read_pin(BSP_GPIO_IMU_GYRO_CS)) {
    BMI088_GYRO_NSS_SET();
    xSemaphoreGiveFromISR(bmi088->sem.gyro_raw, &switch_required);
  }
  portYIELD_FROM_ISR(switch_required);
}

static void bmi088_accl_int_callback(void *arg) {
  bmi088_t *bmi088 = arg;
  BaseType_t switch_required;
  xSemaphoreGiveFromISR(bmi088->sem.accl_new, NULL);
  xSemaphoreGiveFromISR(bmi088->sem.new, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

static void bmi088_gyro_int_callback(void *arg) {
  bmi088_t *bmi088 = arg;
  BaseType_t switch_required;
  xSemaphoreGiveFromISR(bmi088->sem.gyro_new, NULL);
  xSemaphoreGiveFromISR(bmi088->sem.new, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

int8_t bmi088_init(bmi088_t *bmi088, const bmi088_cali_t *cali,
                   const bmi088_param_t *param) {
  ASSERT(bmi088);
  ASSERT(cali);

  if (inited) return DEVICE_ERR_INITED;
  inited = true;

  bmi088->sem.new = xSemaphoreCreateBinary();
  bmi088->sem.gyro_new = xSemaphoreCreateBinary();
  bmi088->sem.accl_new = xSemaphoreCreateBinary();
  bmi088->sem.gyro_raw = xSemaphoreCreateBinary();
  bmi088->sem.accl_raw = xSemaphoreCreateBinary();

  bmi088->cali = cali;
  bmi088->param = param;

  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_SOFTRESET, 0xB6);
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_SOFTRESET, 0xB6);
  bsp_delay(30);

  /* Switch accl to SPI mode. */
  bmi_read_single(BMI_ACCL, BMI088_CHIP_ID_ACCL);

  if (bmi_read_single(BMI_ACCL, BMI088_REG_ACCL_CHIP_ID) != BMI088_CHIP_ID_ACCL)
    return DEVICE_ERR_NO_DEV;

  if (bmi_read_single(BMI_GYRO, BMI088_REG_GYRO_CHIP_ID) != BMI088_CHIP_ID_GYRO)
    return DEVICE_ERR_NO_DEV;

  bsp_gpio_disable_irq(BSP_GPIO_IMU_ACCL_INT);
  bsp_gpio_disable_irq(BSP_GPIO_IMU_GYRO_INT);

  bsp_spi_register_callback(BSP_SPI_IMU, BSP_SPI_RX_CPLT_CB,
                            bmi088_rx_cplt_callback, bmi088);
  bsp_gpio_register_callback(BSP_GPIO_IMU_ACCL_INT, bmi088_accl_int_callback,
                             bmi088);
  bsp_gpio_register_callback(BSP_GPIO_IMU_GYRO_INT, bmi088_gyro_int_callback,
                             bmi088);

  /* Accl init. */
  /* Filter setting: Normal. */
  /* ODR: 0xAB: 800Hz. 0xAA: 400Hz. 0xA9: 200Hz. 0xA8: 100Hz. 0xA6: 25Hz. */
  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_CONF, 0xAA);

  /* 0x00: +-3G. 0x01: +-6G. 0x02: +-12G. 0x03: +-24G. */
  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_RANGE, 0x01);

  /* INT1 as output. Push-pull. Active low. Output. */
  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_INT1_IO_CONF, 0x08);

  /* Map data ready interrupt to INT1. */
  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_INT1_INT2_MAP_DATA, 0x04);

  /* Turn on accl. Now we can read data. */
  bmi_write_single(BMI_ACCL, BMI088_REG_ACCL_PWR_CTRL, 0x04);
  bsp_delay(50);

  /* Gyro init. */
  /* 0x00: +-2000. 0x01: +-1000. 0x02: +-500. 0x03: +-250. 0x04: +-125. */
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_RANGE, 0x01);

  /* Filter bw: 47Hz. */
  /* ODR: 0x02: 1000Hz. 0x03: 400Hz. 0x06: 200Hz. 0x07: 100Hz. */
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_BANDWIDTH, 0x03);

  /* INT3 and INT4 as output. Push-pull. Active low. */
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_CONF, 0x00);

  /* Map data ready interrupt to INT3. */
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_MAP, 0x01);

  /* Enable new data interrupt. */
  bmi_write_single(BMI_GYRO, BMI088_REG_GYRO_INT_CTRL, 0x80);

  bsp_delay(10);

  bsp_gpio_enable_irq(BSP_GPIO_IMU_ACCL_INT);
  bsp_gpio_enable_irq(BSP_GPIO_IMU_GYRO_INT);
  return DEVICE_OK;
}

bool bmi088_wait_new(bmi088_t *bmi088, uint32_t timeout) {
  return xSemaphoreTake(bmi088->sem.new, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

bool bmi088_accl_wait_new(bmi088_t *bmi088, uint32_t timeout) {
  return xSemaphoreTake(bmi088->sem.accl_new, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

bool bmi088_gyro_wait_new(bmi088_t *bmi088, uint32_t timeout) {
  return xSemaphoreTake(bmi088->sem.gyro_new, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

int8_t bmi088_accl_start_dma_recv() {
  bmi_read(BMI_ACCL, BMI088_REG_ACCL_X_LSB, dma_buf, BMI088_ACCL_RX_BUFF_LEN);
  return DEVICE_OK;
}

int8_t bmi088_accl_wait_dma_cplt(bmi088_t *bmi088) {
  xSemaphoreTake(bmi088->sem.accl_raw, portMAX_DELAY);
  return DEVICE_OK;
}

int8_t bmi088_gyro_start_dma_recv() {
  bmi_read(BMI_GYRO, BMI088_REG_GYRO_X_LSB, dma_buf + BMI088_ACCL_RX_BUFF_LEN,
           BMI088_GYRO_RX_BUFF_LEN);
  return DEVICE_OK;
}

int8_t bmi088_gyro_wait_dma_cplt(bmi088_t *bmi088) {
  xSemaphoreTake(bmi088->sem.gyro_raw, portMAX_DELAY);
  return DEVICE_OK;
}

int8_t bmi088_parse_accl(bmi088_t *bmi088) {
  ASSERT(bmi088);

#if 1
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, dma_buf + 1, sizeof(raw_x));
  memcpy(&raw_y, dma_buf + 3, sizeof(raw_y));
  memcpy(&raw_z, dma_buf + 5, sizeof(raw_z));

  bmi088->accl.x = (float)raw_x;
  bmi088->accl.y = (float)raw_y;
  bmi088->accl.z = (float)raw_z;

#else
  const int16_t *praw_x = (int16_t *)(dma_buf + 1);
  const int16_t *praw_y = (int16_t *)(dma_buf + 3);
  const int16_t *praw_z = (int16_t *)(dma_buf + 5);

  bmi088->accl.x = (float)*praw_x;
  bmi088->accl.y = (float)*praw_y;
  bmi088->accl.z = (float)*praw_z;

#endif

  /* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
  bmi088->accl.x /= 5460.0f;
  bmi088->accl.y /= 5460.0f;
  bmi088->accl.z /= 5460.0f;

  int16_t raw_temp = (int16_t)((dma_buf[17] << 3) | (dma_buf[18] >> 5));

  if (raw_temp > 1023) raw_temp -= 2048;

  bmi088->temp = (float)raw_temp * 0.125f + 23.0f;

  if (bmi088->param->inverted) {
    bmi088->accl.x = -bmi088->accl.x;
    bmi088->accl.z = -bmi088->accl.z;
  }

  return DEVICE_OK;
}

int8_t bmi088_parse_gyro(bmi088_t *bmi088) {
  ASSERT(bmi088);

#if 1
  /* Gyroscope imu_raw -> degrees/sec -> radians/sec */
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, dma_buf + BMI088_ACCL_RX_BUFF_LEN, sizeof(raw_x));
  memcpy(&raw_y, dma_buf + BMI088_ACCL_RX_BUFF_LEN + 2, sizeof(raw_y));
  memcpy(&raw_z, dma_buf + BMI088_ACCL_RX_BUFF_LEN + 4, sizeof(raw_z));

  bmi088->gyro.x = (float)raw_x;
  bmi088->gyro.y = (float)raw_y;
  bmi088->gyro.z = (float)raw_z;

#else
  /* Gyroscope imu_raw -> degrees/sec -> radians/sec */
  const int16_t *raw_x = (int16_t *)(dma_buf + BMI088_ACCL_RX_BUFF_LEN);
  const int16_t *raw_y = (int16_t *)(dma_buf + BMI088_ACCL_RX_BUFF_LEN + 2);
  const int16_t *raw_z = (int16_t *)(dma_buf + BMI088_ACCL_RX_BUFF_LEN + 4);

  bmi088->gyro.x = (float)*raw_x;
  bmi088->gyro.y = (float)*raw_y;
  bmi088->gyro.z = (float)*raw_z;
#endif

  /* FS125: 262.144. FS250: 131.072. FS500: 65.536. FS1000: 32.768.
   * FS2000: 16.384.*/
  bmi088->gyro.x /= 32.768f;
  bmi088->gyro.y /= 32.768f;
  bmi088->gyro.z /= 32.768f;

  bmi088->gyro.x *= M_DEG2RAD_MULT;
  bmi088->gyro.y *= M_DEG2RAD_MULT;
  bmi088->gyro.z *= M_DEG2RAD_MULT;

  if (bmi088->param->inverted) {
    bmi088->gyro.x = -bmi088->gyro.x;
    bmi088->gyro.z = -bmi088->gyro.z;
  }

  bmi088->gyro.x -= bmi088->cali->gyro_offset.x;
  bmi088->gyro.y -= bmi088->cali->gyro_offset.y;
  bmi088->gyro.z -= bmi088->cali->gyro_offset.z;

  return DEVICE_OK;
}

float bmi088_get_update_freq(bmi088_t *bmi088) {
  RM_UNUSED(bmi088);
  return 400.0f;
}
