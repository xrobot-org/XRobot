#include "dev_bq27220.hpp"

#include "bsp_i2c.h"

#define BQ27220_DEVICE_ID (0x55)
#define CTRL_REG_ADDR (0x00)
#define STATE_OF_CHARGE_ADDR (0x2C)
#define OPERATION_STATUS_ADDR (0x3B)
#define DESIGN_CAPACITY_CTRL_ADDR (0x3E)
#define DESIGN_CAPACITY_ADDR (0x40)
#define CAPACITY_DATA_SUM_ADDR (0x60)
#define CAPACITY_DATA_SUM_LEN_ADDR (0x61)
#define TEMPERATURE_ADDR (0x06)

using namespace Device;

BQ27220::BQ27220(Param& param) : param_(param), cmd_(this, CMD, "bq27220") {
  static uint8_t old_capacity[2];

  /* Read old capacity */
  for (int i = 0; i < 6; i++) {
    old_capacity[0] =
        bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, 0X3C);
    System::Thread::Sleep(50);
    old_capacity[1] =
        bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, 0X3C + 1);
    System::Thread::Sleep(50);

    auto old_cap = *reinterpret_cast<uint16_t*>(old_capacity);

    if (old_cap == 3000 || old_cap == param.capacity_mha) {
      break;
    }
  }

  old_capacity[0] =
      bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, 0X3C);
  System::Thread::Sleep(50);
  old_capacity[1] =
      bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, 0X3C + 1);
  System::Thread::Sleep(50);

  auto old_cap = *reinterpret_cast<uint16_t*>(old_capacity);
  if (old_cap != param_.capacity_mha) {
    WriteParam();
  }

  auto task_fun = [](BQ27220* bq27220) {
    static uint8_t tmp[10];
    int16_t* data = reinterpret_cast<int16_t*>(tmp);

    bsp_i2c_mem_read(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, STATE_OF_CHARGE_ADDR,
                     tmp, 2, true);
    bq27220->data_.percentage = data[0];
    bsp_i2c_mem_read(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, TEMPERATURE_ADDR, tmp,
                     10, true);
    bq27220->data_.temperate = static_cast<float>(data[0] - 2731) / 10.0f;
    bq27220->data_.voltage = static_cast<float>(data[1]) / 1000.0f;
    bq27220->data_.current = static_cast<float>(data[3]) / 1000.0f;

    bq27220->data_.real_capacity = data[2];
  };

  System::Timer::Create(task_fun, this, 1000);
}

void BQ27220::WriteParam() {
  // TODO:Param write Invalid
  /* UNSEAL it by sending the appropriate keys to Control */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0X14);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0X04);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0X72);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0X36);

  /* Enter FULL ACCESS */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0XFF);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0XFF);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0XFF);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0XFF);

  /* Send ENTER_CGF_UPDATE command */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0X90);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0X00);

  /* Wait CFGUPDATE, May take up to 1 second. */
  uint8_t tmp = 0;

  do {
    tmp = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                OPERATION_STATUS_ADDR);
  } while ((tmp & (1 << 2)) != (1 << 2));

  /* Enable access of Design Capacity */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         DESIGN_CAPACITY_CTRL_ADDR, 0x9F);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         DESIGN_CAPACITY_CTRL_ADDR + 1, 0x92);

  /* Read MACDataSum and MACDataLen */
  static uint8_t old_cap_sum_data, old_cap_sum_len;
  old_cap_sum_data = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                           CAPACITY_DATA_SUM_ADDR);
  old_cap_sum_len = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                          CAPACITY_DATA_SUM_LEN_ADDR);

  /* Read old capacity */
  static uint8_t old_capacity[2];
  old_capacity[1] = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                          DESIGN_CAPACITY_ADDR);
  old_capacity[0] = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                          DESIGN_CAPACITY_ADDR + 1);

  auto old_cap = *reinterpret_cast<uint16_t*>(old_capacity);
  if (old_cap == param_.capacity_mha) {
    return;
  }

  /* Write new capacity */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         DESIGN_CAPACITY_ADDR, param_.capacity_mha >> 8);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         DESIGN_CAPACITY_ADDR + 1, param_.capacity_mha);

  /* Write new sum */
  uint8_t new_sum =
      (255 - old_cap_sum_data - old_capacity[0] - old_capacity[1]) % 256;
  new_sum = 255 - ((new_sum + 0x04 + 0xb0) % 256);

  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         CAPACITY_DATA_SUM_ADDR, new_sum);

  /* Write new length */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                         CAPACITY_DATA_SUM_LEN_ADDR, 0x24);

  /* Exit CFGUPDATE mode */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0X92);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0X00);

  /* Wait CFGUPDATE, May take up to 1 second. */

  do {
    tmp = bsp_i2c_mem_read_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID,
                                OPERATION_STATUS_ADDR);
  } while ((tmp & (1 << 2)) != 0);

  /* return to SEALED mode */
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR,
                         0X30);
  bsp_i2c_mem_write_byte(BSP_I2C_BQ27220, BQ27220_DEVICE_ID, CTRL_REG_ADDR + 1,
                         0X00);
}

int BQ27220::CMD(BQ27220* bq27220, int argc, char** argv) {
  XB_UNUSED(argc);
  XB_UNUSED(argv);
  printf("%d percentage Current:%f ma Voltage:%f V Tempreature:%f °c\r\n",
         bq27220->data_.percentage, bq27220->data_.current,
         bq27220->data_.voltage, bq27220->data_.temperate);
  return 0;
}
