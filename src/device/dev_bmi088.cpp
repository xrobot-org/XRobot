/*
        BMI088 陀螺仪+加速度计传感器。

*/

#include "dev_bmi088.hpp"

#include <stdbool.h>
#include <string.h>

#include "bsp_delay.h"
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "bsp_spi.h"
#include "comp_pid.hpp"
#include "comp_utils.hpp"
#include "om.h"

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

static uint8_t tx_rx_buf[2];
static uint8_t dma_buf[BMI088_ACCL_RX_BUFF_LEN + BMI088_GYRO_RX_BUFF_LEN];
static Component::PID::Param imu_temp_ctrl_pid_param = {
    .k = 0.15f,
    .p = 1.0f,
    .i = 0.0f,
    .d = 0.0f,
    .i_limit = 1.0f,
    .out_limit = 1.0f,
    .d_cutoff_freq = 0.0f,
    .range = 0.0f,
};

using namespace Device;

void BMI088::Select(BMI088::DeviceType type) {
  if (type == BMI_ACCL) {
    bsp_gpio_write_pin(BSP_GPIO_IMU_ACCL_CS, false);
  } else {
    bsp_gpio_write_pin(BSP_GPIO_IMU_GYRO_CS, false);
  }
}

void BMI088::Unselect(BMI088::DeviceType type) {
  if (type == BMI_ACCL) {
    bsp_gpio_write_pin(BSP_GPIO_IMU_ACCL_CS, true);
  } else {
    bsp_gpio_write_pin(BSP_GPIO_IMU_GYRO_CS, true);
  }
}

void BMI088::WriteSingle(BMI088::DeviceType type, uint8_t reg, uint8_t data) {
  tx_rx_buf[0] = (reg & 0x7f);
  tx_rx_buf[1] = data;

  bsp_delay(1);

  this->Select(type);
  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 2u, true);
  this->Unselect(type);
}

uint8_t BMI088::ReadSingle(BMI088::DeviceType type, uint8_t reg) {
  tx_rx_buf[0] = (uint8_t)(reg | 0x80);

  bsp_delay(1);

  this->Select(type);
  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 1u, true);
  bsp_spi_receive(BSP_SPI_IMU, tx_rx_buf, 2u, true);
  this->Unselect(type);
  if (type == BMI088::BMI_ACCL)
    return tx_rx_buf[1];
  else
    return tx_rx_buf[0];
}

void BMI088::Read(BMI088::DeviceType type, uint8_t reg, uint8_t *data,
                  uint8_t len) {
  tx_rx_buf[0] = (uint8_t)(reg | 0x80);

  this->Select(type);
  bsp_spi_transmit(BSP_SPI_IMU, tx_rx_buf, 1u, true);
  bsp_spi_receive(BSP_SPI_IMU, data, len, false);
}

BMI088::BMI088(BMI088::Calibration &cali, BMI088::Rotation &rot)
    : cali(cali), rot(rot) {
  auto recv_cplt_callback = [](void *arg) {
    BMI088 *bmi088 = (BMI088 *)arg;

    if (!bsp_gpio_read_pin(BSP_GPIO_IMU_ACCL_CS)) {
      bmi088->Unselect(BMI_ACCL);
      bmi088->sem.accl_raw_.GiveFromISR();
    }
    if (!bsp_gpio_read_pin(BSP_GPIO_IMU_GYRO_CS)) {
      bmi088->Unselect(BMI_GYRO);
      bmi088->sem.gyro_raw_.GiveFromISR();
    }
  };

  auto accl_int_callback = [](void *arg) {
    BMI088 *bmi088 = (BMI088 *)arg;
    bmi088->sem.accl_new_.GiveFromISR();
    bmi088->sem.new_.GiveFromISR();
  };

  auto gyro_int_callback = [](void *arg) {
    BMI088 *bmi088 = (BMI088 *)arg;
    bmi088->sem.gyro_new_.GiveFromISR();
    bmi088->sem.new_.GiveFromISR();
  };

  bsp_gpio_disable_irq(BSP_GPIO_IMU_ACCL_INT);
  bsp_gpio_disable_irq(BSP_GPIO_IMU_GYRO_INT);

  bsp_gpio_register_callback(BSP_GPIO_IMU_ACCL_INT, accl_int_callback, this);
  bsp_gpio_register_callback(BSP_GPIO_IMU_GYRO_INT, gyro_int_callback, this);

  bsp_spi_register_callback(BSP_SPI_IMU, BSP_SPI_RX_CPLT_CB, recv_cplt_callback,
                            this);

  auto thread_bmi088 = [](void *arg) {
    BMI088 *bmi088 = (BMI088 *)arg;

    Component::PID imu_temp_ctrl_pid(imu_temp_ctrl_pid_param, 1000.0f);

    DECLARE_TOPIC(accl_tp, bmi088->data.accl_, "gimbal_accl", true);
    DECLARE_TOPIC(gyro_tp, bmi088->data.gyro_, "gimbal_gyro", true);

    bsp_pwm_start(BSP_PWM_IMU_HEAT);

    while (!bmi088->Init()) {
      bmi088->thread_.Sleep(1);
    }

    while (1) {
      /* 开始数据接收DMA，加速度计和陀螺仪共用同一个SPI接口，
       * 一次只能开启一个DMA
       */
      if (bmi088->sem.new_.Take(UINT32_MAX)) {
        if (bmi088->sem.accl_new_.Take(0)) {
          bmi088->StartRecvAccel();
          bmi088->sem.accl_raw_.Take(UINT16_MAX);
          bmi088->PraseAccel();

          accl_tp.Publish();
        }

        if (bmi088->sem.gyro_new_.Take(0)) {
          bmi088->StartRecvGyro();
          bmi088->sem.gyro_raw_.Take(UINT16_MAX);
          bmi088->PraseGyro();

          gyro_tp.Publish();
        }
      }

      // TODO: 添加滤波

      /* PID控制IMU温度，PWM输出 */
      bsp_pwm_set_comp(BSP_PWM_IMU_HEAT, imu_temp_ctrl_pid.Calculate(
                                             40.0f, bmi088->data.temp_, 0));
      bmi088->thread_.Sleep(1);
    }
  };

  THREAD_DECLEAR(this->thread_, thread_bmi088, 256, System::Thread::Realtime,
                 this);
}

bool BMI088::Init() {
  /* BMI088软件重启 */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_SOFTRESET, 0xB6);
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_SOFTRESET, 0xB6);
  bsp_delay(30);

  ReadSingle(BMI_ACCL, BMI088_CHIP_ID_ACCL);

  if (ReadSingle(BMI_ACCL, BMI088_REG_ACCL_CHIP_ID) != BMI088_CHIP_ID_ACCL)
    return false;

  if (ReadSingle(BMI_GYRO, BMI088_REG_GYRO_CHIP_ID) != BMI088_CHIP_ID_GYRO)
    return false;

  /* Accl init. */
  /* Filter setting: Normal. */
  /* ODR: 0xAB: 800Hz. 0xAA: 400Hz. 0xA9: 200Hz. 0xA8: 100Hz. 0xA6: 25Hz. */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_CONF, 0xAB);

  /* 0x00: +-3G. 0x01: +-6G. 0x02: +-12G. 0x03: +-24G. */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_RANGE, 0x01);

  /* INT1 as output. Push-pull. Active low. Output. */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_INT1_IO_CONF, 0x08);

  /* Map data ready interrupt to INT1. */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_INT1_INT2_MAP_DATA, 0x04);

  /* Turn on accl. Now we can read data. */
  WriteSingle(BMI_ACCL, BMI088_REG_ACCL_PWR_CTRL, 0x04);
  bsp_delay(50);

  /* Gyro init. */
  /* 0x00: +-2000. 0x01: +-1000. 0x02: +-500. 0x03: +-250. 0x04: +-125. */
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_RANGE, 0x01);

  /* Filter bw: 47Hz. */
  /* ODR: 0x02: 1000Hz. 0x03: 400Hz. 0x06: 200Hz. 0x07: 100Hz. */
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_BANDWIDTH, 0x03);

  /* INT3 and INT4 as output. Push-pull. Active low. */
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_CONF, 0x00);

  /* Map data ready interrupt to INT3. */
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT3_INT4_IO_MAP, 0x01);

  /* Enable new data interrupt. */
  WriteSingle(BMI_GYRO, BMI088_REG_GYRO_INT_CTRL, 0x80);

  bsp_delay(30);

  bsp_gpio_enable_irq(BSP_GPIO_IMU_ACCL_INT);
  bsp_gpio_enable_irq(BSP_GPIO_IMU_GYRO_INT);

  return true;
}

void BMI088::PraseGyro() {
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, dma_buf + BMI088_ACCL_RX_BUFF_LEN, sizeof(raw_x));
  memcpy(&raw_y, dma_buf + BMI088_ACCL_RX_BUFF_LEN + 2, sizeof(raw_y));
  memcpy(&raw_z, dma_buf + BMI088_ACCL_RX_BUFF_LEN + 4, sizeof(raw_z));

  float gyro[3] = {(float)raw_x, (float)raw_y, (float)raw_z};

  /* FS125: 262.144. FS250: 131.072. FS500: 65.536. FS1000: 32.768.
   * FS2000: 16.384.*/

  for (int i = 0; i < 3; i++) {
    gyro[i] /= 32.768f;
    gyro[i] *= M_DEG2RAD_MULT;
  }

  memset(&(this->data.gyro_), 0, sizeof(this->data.gyro_));

  for (int i = 0; i < 3; i++) {
    this->data.gyro_.x += this->rot.rot_mat[0][i] * gyro[i];
    this->data.gyro_.y += this->rot.rot_mat[1][i] * gyro[i];
    this->data.gyro_.z += this->rot.rot_mat[2][i] * gyro[i];
  }

  this->data.gyro_.x -= this->cali.gyro_offset.x;
  this->data.gyro_.y -= this->cali.gyro_offset.y;
  this->data.gyro_.z -= this->cali.gyro_offset.z;
}

void BMI088::PraseAccel() {
  int16_t raw_x, raw_y, raw_z;
  memcpy(&raw_x, dma_buf + 1, sizeof(raw_x));
  memcpy(&raw_y, dma_buf + 3, sizeof(raw_y));
  memcpy(&raw_z, dma_buf + 5, sizeof(raw_z));

  float accl[3] = {(float)raw_x, (float)raw_y, (float)raw_z};

  /* 3G: 10920. 6G: 5460. 12G: 2730. 24G: 1365. */
  for (int i = 0; i < 3; i++) {
    accl[i] /= 5640.0f;
  }

  int16_t raw_temp = (int16_t)((dma_buf[17] << 3) | (dma_buf[18] >> 5));

  if (raw_temp > 1023) raw_temp -= 2048;

  this->data.temp_ = (float)raw_temp * 0.125f + 23.0f;

  memset(&(this->data.accl_), 0, sizeof(this->data.accl_));

  for (int i = 0; i < 3; i++) {
    this->data.accl_.x += this->rot.rot_mat[0][i] * accl[i];
    this->data.accl_.y += this->rot.rot_mat[1][i] * accl[i];
    this->data.accl_.z += this->rot.rot_mat[2][i] * accl[i];
  }
}

bool BMI088::StartRecvGyro() {
  Read(BMI_GYRO, BMI088_REG_GYRO_X_LSB, dma_buf + BMI088_ACCL_RX_BUFF_LEN,
       BMI088_GYRO_RX_BUFF_LEN);
  return true;
}

bool BMI088::StartRecvAccel() {
  Read(BMI_ACCL, BMI088_REG_ACCL_X_LSB, dma_buf, BMI088_ACCL_RX_BUFF_LEN);
  return true;
}
