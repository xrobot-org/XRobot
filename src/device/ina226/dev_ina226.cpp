#include "dev_ina226.hpp"

#include "bsp_i2c.h"

#define INA226_CALIB_VAL 1024
#define INA226_CURRENTLSB 0.5F                            // mA/bit
#define INA226_CURRENTLSB_INV 1 / INA226_CURRENTLSB       // bit/mA
#define INA226_POWERLSB_INV 1 / (INA226_CURRENTLSB * 25)  // bit/mW

#define INA226_CONFIG 0x00  // Configuration Register (R/W)初始值4127
#define INA226_SHUNTV 0x01  // Shunt Voltage (R)初始值0，分流电压测量值
#define INA226_BUSV 0x02    // Bus Voltage (R)初始值0，总线电压测量值
#define INA226_POWER 0x03  // Power (R)初始值0，输出功率测量值
#define INA226_CURRENT 0x04  // Current (R)初始值0，分流电阻电流计算值
#define INA226_CALIB 0x05  // Calibration (R/W)，设置全量程和电流LSB
#define INA226_MASK 0x06  // Mask/Enable (R/W)，报警设置和转换准备标志
#define INA226_ALERTL 0x07    // Alert Limit (R/W)，报警阈值
#define INA226_MANUF_ID 0xFE  // Manufacturer ID (R)，0x5449
#define INA226_DIE_ID 0xFF    // Die ID (R),0x2260

#define INA226_MODE_POWER_DOWN (0 << 0)          // Power-Down
#define INA226_MODE_TRIG_SHUNT_VOLTAGE (1 << 0)  // Shunt Voltage, Triggered
#define INA226_MODE_TRIG_BUS_VOLTAGE (2 << 0)    // Bus Voltage, Triggered
#define INA226_MODE_TRIG_SHUNT_AND_BUS (3 << 0)  // Shunt and Bus, Triggered
#define INA226_MODE_POWER_DOWN2 (4 << 0)         // Power-Down
#define INA226_MODE_CONT_SHUNT_VOLTAGE (5 << 0)  // Shunt Voltage, Continuous
#define INA226_MODE_CONT_BUS_VOLTAGE (6 << 0)    // Bus Voltage, Continuous
#define INA226_MODE_CONT_SHUNT_AND_BUS (7 << 0)  // Shunt and Bus, Continuous

// Shunt Voltage Conversion Time
#define INA226_VSH_140uS (0 << 3)
#define INA226_VSH_204uS (1 << 3)
#define INA226_VSH_332uS (2 << 3)
#define INA226_VSH_588uS (3 << 3)
#define INA226_VSH_1100uS (4 << 3)
#define INA226_VSH_2116uS (5 << 3)
#define INA226_VSH_4156uS (6 << 3)
#define INA226_VSH_8244uS (7 << 3)

// Bus Voltage Conversion Time (VBUS CT Bit Settings[6-8])
#define INA226_VBUS_140uS (0 << 6)
#define INA226_VBUS_204uS (1 << 6)
#define INA226_VBUS_332uS (2 << 6)
#define INA226_VBUS_588uS (3 << 6)
#define INA226_VBUS_1100uS (4 << 6)
#define INA226_VBUS_2116uS (5 << 6)
#define INA226_VBUS_4156uS (6 << 6)
#define INA226_VBUS_8244uS (7 << 6)

// Averaging Mode (AVG Bit Settings[9-11])
#define INA226_AVG_1 (0 << 9)
#define INA226_AVG_4 (1 << 9)
#define INA226_AVG_16 (2 << 9)
#define INA226_AVG_64 (3 << 9)
#define INA226_AVG_128 (4 << 9)
#define INA226_AVG_256 (5 << 9)
#define INA226_AVG_512 (6 << 9)
#define INA226_AVG_1024 (7 << 9)

// Reset Bit (RST bit [15])
#define INA226_RESET_ACTIVE (1 << 15)
#define INA226_RESET_INACTIVE (0 << 15)

// Mask/Enable Register
#define INA226_MER_SOL (1 << 15)   // Shunt Voltage Over-Voltage
#define INA226_MER_SUL (1 << 14)   // Shunt Voltage Under-Voltage
#define INA226_MER_BOL (1 << 13)   // Bus Voltagee Over-Voltage
#define INA226_MER_BUL (1 << 12)   // Bus Voltage Under-Voltage
#define INA226_MER_POL (1 << 11)   // Power Over-Limit
#define INA226_MER_CNVR (1 << 10)  // Conversion Ready
#define INA226_MER_AFF (1 << 4)    // Alert Function Flag
#define INA226_MER_CVRF (1 << 3)   // Conversion Ready Flag
#define INA226_MER_OVF (1 << 2)    // Math Overflow Flag
#define INA226_MER_APOL (1 << 1)   // Alert Polarity Bit
#define INA226_MER_LEN (1 << 0)    // Alert Latch Enable

#define INA226_MANUF_ID_DEFAULT 0x5449
#define INA226_DIE_ID_DEFAULT 0x2260

using namespace Device;

static uint8_t i2c_buff[8] = {0};

Ina226::Ina226(Param& param)
    : param_(param),
      current_offset_("ina226_offset"),
      cmd_(this, Cali, "ina226_cali"),
      max_current_(param.resistance * 81.92f),
      current_lsb_(max_current_ / 32768.0f),
      power_lsb_(current_lsb_ * 25),
      cali_(static_cast<uint32_t>(0.00512f / current_lsb_ / param.resistance *
                                  1000.0f)),
      info_tp_("ina226_info") {
  bsp_i2c_mem_read(param.i2c, param.device_id, INA226_MANUF_ID, i2c_buff, 2,
                   true);
  while (strcmp(reinterpret_cast<char*>(i2c_buff), "TI") != 0) {
    OMLOG_ERROR("INA226 Manufacturer ID read error");
    System::Thread::Sleep(100);
    bsp_i2c_mem_read(param.i2c, param.device_id, INA226_MANUF_ID, i2c_buff, 2,
                     true);
  }

  /* 0x4407          010    000          000         111                   */
  /* Config Register AVG-16 VBUSCT-140us VSHCT-140us Mode-Shunt Continuous */
  *reinterpret_cast<uint16_t*>(i2c_buff) = 0x0744;
  bsp_i2c_mem_write(param.i2c, param.device_id, INA226_CONFIG, i2c_buff, 2,
                    true);

  /* Calibration Register */
  *reinterpret_cast<uint16_t*>(i2c_buff) =
      ((cali_ & 0xff) << 8 | (cali_ & 0xff00) >> 8);
  bsp_i2c_mem_write(param.i2c, param.device_id, INA226_CALIB, i2c_buff, 2,
                    true);

  auto ina_task = [](Ina226* ina) {
    ina->GetData();
    ina->info_tp_.Publish(ina->info_);
  };
  timer_ = System::Timer::Create(ina_task, this, 10);
}

void Ina226::GetData() {
  bsp_i2c_mem_read(param_.i2c, param_.device_id, INA226_SHUNTV, i2c_buff, 2,
                   true);
  bsp_i2c_mem_read(param_.i2c, param_.device_id, INA226_BUSV, i2c_buff + 2, 2,
                   true);
  bsp_i2c_mem_read(param_.i2c, param_.device_id, INA226_POWER, i2c_buff + 4, 2,
                   true);
  bsp_i2c_mem_read(param_.i2c, param_.device_id, INA226_CURRENT, i2c_buff + 6,
                   2, true);
  info_.shunt_volt =
      static_cast<float>(i2c_buff[0] << 8 | i2c_buff[1]) * 0.0000025f;
  info_.bus_volt =
      static_cast<float>(i2c_buff[2] << 8 | i2c_buff[3]) * 0.00125f;
  info_.current =
      static_cast<float>(i2c_buff[6] << 8 | i2c_buff[7]) * current_lsb_ -
      current_offset_;
  info_.power = static_cast<float>(i2c_buff[4] << 8 | i2c_buff[5]) * power_lsb_;
}

int Ina226::Cali(Ina226* ina, int argc, char** argv) {
  XB_UNUSED(argc);
  XB_UNUSED(argv);

  printf("Start cali ina226 current\r\n");
  System::Timer::Stop(ina->timer_);
  ina->current_offset_.data_ = 0;

  float offset = 0.0f;

  for (int i = 0; i < 100; i++) {
    ina->GetData();
    offset += ina->info_.current * 0.01f;
    printf("%d/100", i);
    System::Thread::Sleep(10);
    ms_clear_line();
  }

  ina->current_offset_.Set(offset);

  System::Timer::Start(ina->timer_);

  return 0;
}
