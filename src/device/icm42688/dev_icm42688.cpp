/*
        ICM42688 陀螺仪+加速度计传感器。 参考：https://github.com/Q1anWan/GY-H1

*/

#include "dev_icm42688.hpp"

#include <string_view>

#include "bsp_def.h"
#include "bsp_gpio.h"
#include "bsp_spi.h"
#include "bsp_time.h"
#include "comp_pid.hpp"

static uint8_t dma_buf[14];

using namespace Device;

void ICM42688::WriteSingle(uint8_t reg, uint8_t data) {
  System::Thread::Sleep(1);
  this->Select();
  bsp_spi_mem_write_byte(BSP_SPI_IMU, reg, data);
  this->Unselect();
}

uint8_t ICM42688::ReadSingle(uint8_t reg) {
  System::Thread::Sleep(1);
  this->Select();
  reg = bsp_spi_mem_read_byte(BSP_SPI_IMU, reg);
  this->Unselect();
  return reg;
}

void ICM42688::Read(uint8_t reg, uint8_t *data, uint8_t len) {
  this->Select();
  bsp_spi_mem_read(BSP_SPI_IMU, reg, data, len, false);
}

ICM42688::ICM42688(ICM42688::Rotation &rot)
    : cali_("icm42688_cali"),
      rot_(rot),
      raw_(0),
      new_(0),
      accl_tp_("imu_accl"),
      gyro_tp_("imu_gyro"),
      cmd_(this, this->CaliCMD, "icm42688") {
  auto recv_cplt_callback = [](void *arg) {
    ICM42688 *icm42688 = static_cast<ICM42688 *>(arg);
    icm42688->Unselect();
    icm42688->raw_.Post();
  };

  auto int_callback = [](void *arg) {
    ICM42688 *icm42688 = static_cast<ICM42688 *>(arg);
    icm42688->new_.Post();
  };

  bsp_gpio_disable_irq(BSP_GPIO_IMU_INT_1);
  bsp_gpio_disable_irq(BSP_GPIO_IMU_INT_2);

  bsp_gpio_register_callback(BSP_GPIO_IMU_INT_1, int_callback, this);
  bsp_gpio_register_callback(BSP_GPIO_IMU_INT_2, int_callback, this);

  bsp_spi_register_callback(BSP_SPI_IMU, BSP_SPI_RX_CPLT_CB, recv_cplt_callback,
                            this);

  auto thread_icm42688 = [](ICM42688 *icm42688) {
    while (!icm42688->Init()) {
      System::Thread::Sleep(1);
    }

    while (1) {
      /* 开始数据接收DMA，加速度计和陀螺仪共用同一个SPI接口，
       * 一次只能开启一个DMA
       */
      if (icm42688->new_.Wait(20)) {
        icm42688->StartRecv();
        icm42688->raw_.Wait(UINT32_MAX);
        icm42688->Prase();

        icm42688->accl_tp_.Publish(icm42688->accl_);
        icm42688->gyro_tp_.Publish(icm42688->gyro_);

      } else {
        OMLOG_ERROR("ICM42688 wait timeout.");
      }
    }
  };

  this->thread_accl_.Create(thread_icm42688, this, "thread_icm4268",
                            DEVICE_ICM42688_TASK_STACK_DEPTH,
                            System::Thread::REALTIME);
}

int ICM42688::CaliCMD(ICM42688 *icm42688, int argc, char **argv) {
  if (argc == 1) {
    printf("show [time] [delay] 在time时间内每隔delay打印一次数据\r\n");
    printf("list 列出校准数据\r\n");
    printf("cali 开始校准\r\n");
  } else if (argc == 2) {
    if (strcmp(argv[1], "list") == 0) {
      printf("校准数据 x:%f y:%f z:%f\r\n", icm42688->cali_.data_.gyro_offset.x,
             icm42688->cali_.data_.gyro_offset.y,
             icm42688->cali_.data_.gyro_offset.z);
    } else if (strcmp(argv[1], "cali") == 0) {
      printf("开始校准，请保持陀螺仪稳定\r\n");
      double x = 0.0f, y = 0.0f, z = 0.0f;
      for (int i = 0; i < 30000; i++) {
        x += static_cast<double>(icm42688->gyro_.x) / 30000.0f;
        y += static_cast<double>(icm42688->gyro_.y) / 30000.0f;
        z += static_cast<double>(icm42688->gyro_.z) / 30000.0f;
        printf("进度：%d/30000", i);
        System::Thread::Sleep(1);
        ms_clear_line();
      }

      icm42688->cali_.data_.gyro_offset.x += static_cast<float>(x);
      icm42688->cali_.data_.gyro_offset.y += static_cast<float>(y);
      icm42688->cali_.data_.gyro_offset.z += static_cast<float>(z);

      printf("校准数据 x:%f y:%f z:%f\r\n", icm42688->cali_.data_.gyro_offset.x,
             icm42688->cali_.data_.gyro_offset.y,
             icm42688->cali_.data_.gyro_offset.z);

      x = y = z = 0.0f;

      printf("开始分析校准质量\r\n");

      for (int i = 0; i < 30000; i++) {
        x += static_cast<double>(icm42688->gyro_.x) / 30000.0f;
        y += static_cast<double>(icm42688->gyro_.y) / 30000.0f;
        z += static_cast<double>(icm42688->gyro_.z) / 30000.0f;
        printf("进度：%d/30000", i);
        System::Thread::Sleep(1);
        ms_clear_line();
      }

      printf("校准误差: x:%f y:%f z:%f\r\n", x, y, z);

      icm42688->cali_.Set();

      printf("保存校准数据\r\n");

      printf("完成\r\n");
    }
  } else if (argc == 4) {
    if (strcmp(argv[1], "show") == 0) {
      int time = std::stoi(argv[2]);
      int delay = std::stoi(argv[3]);

      if (delay > 1000) {
        delay = 1000;
      }
      if (delay < 2) {
        delay = 2;
      }

      do {
        printf("accl x:%+5f y:%+5f z:%+5f gyro x:%+5f y:%+5f z:%+5f",
               icm42688->accl_.x, icm42688->accl_.y, icm42688->accl_.z,
               icm42688->gyro_.x, icm42688->gyro_.y, icm42688->gyro_.z);
        System::Thread::Sleep(delay);
        ms_clear_line();
        time -= delay;
      } while (time > delay);

      printf("\r\n");
    }
  } else {
    printf("参数错误\r\n");
    return -1;
  }

  return 0;
}

bool ICM42688::Init() {
  /*指定Bank0*/
  WriteSingle(0x76, 0x00);
  /*软重启*/
  WriteSingle(0x11, 0x01);
  System::Thread::Sleep(5);
  /*读取中断位 切换SPI*/
  auto buf = ReadSingle(0x2D);
  XB_UNUSED(buf);
  /*指定Bank0*/
  WriteSingle(0x76, 0x00);
  /* 检查WhoAmI */
  buf = ReadSingle(0x75);
  while (buf != 0x47) {
    OMLOG_ERROR("ICM42688 WhoAmI is %d, retry", buf);
    return false;
  }
  /*中断输出设置*/
  WriteSingle(0x14, 0x12);  // INT1 INT2 脉冲模式，低有效
  /*Gyro设置*/
  WriteSingle(0x4F, 0x06);  // 2000dps 1KHz
  /*Accel设置*/
  WriteSingle(0x50, 0x06);  // 16G 1KHz
  /*Tem设置&Gyro_Config1*/
  WriteSingle(0x51, 0x56);  // BW 82Hz Latency = 2ms
  /*GYRO_ACCEL_CONFIG0*/
  WriteSingle(0x52, 0x11);  // 1BW
  /*ACCEL_CONFIG1*/
  WriteSingle(0x53, 0x0D);  // Null
  /*INT_CONFIG0*/
  WriteSingle(0x63, 0x00);  // Null
  /*INT_CONFIG1*/
  WriteSingle(0x64, 0x00);  //中断引脚正常启用
  /*INT_SOURCE0*/
  WriteSingle(0x65, 0x08);  // DRDY INT1
  /*INT_SOURCE1*/
  WriteSingle(0x66, 0x00);  // Null
  /*INT_SOURCE3*/
  WriteSingle(0x68, 0x00);  // Null
  /*INT_SOURCE3*/
  WriteSingle(0x69, 0x00);  // Null

  /*****抗混叠滤波器@536Hz*****/

  /*GYRO抗混叠滤波器配置*/
  /*指定Bank1*/
  WriteSingle(0x76, 0x01);
  /*GYRO抗混叠滤波器配置*/
  WriteSingle(0x0B, 0xA0);  //开启抗混叠和陷波滤波器
  WriteSingle(0x0C, 0x0C);  // GYRO_AAF_DELT 12 (default 13)
  WriteSingle(0x0D, 0x90);  // GYRO_AAF_DELTSQR 144 (default 170)
  WriteSingle(0x0E, 0x80);  // GYRO_AAF_BITSHIFT 8 (default 8)

  /*ACCEL抗混叠滤波器配置*/
  /*指定Bank2*/
  WriteSingle(0x76, 0x02);
  /*ACCEL抗混叠滤波器配置*/
  WriteSingle(0x03, 0x18);  //开启滤波器 ACCEL_AFF_DELT 12 (default 24)
  WriteSingle(0x04, 0x90);  // ACCEL_AFF_DELTSQR 144 (default 64)
  WriteSingle(0x05, 0x80);  // ACCEL_AAF_BITSHIFT 8 (default 6)

  /*****自定义滤波器1号@111Hz*****/

  /*指定Bank0*/
  WriteSingle(0x76, 0x00);
  /*滤波器顺序*/
  WriteSingle(0x51, 0x12);  // GYRO滤波器1st
  WriteSingle(0x53, 0x05);  // ACCEL滤波器1st
  /*滤波器设置*/
  WriteSingle(0x52, 0x33);  // 111Hz 03

  /*指定Bank0*/
  WriteSingle(0x76, 0x00);
  /*电源管理*/
  WriteSingle(0x4E, 0x0F);  // ACC GYRO LowNoise Mode

  System::Thread::Sleep(50);
  bsp_gpio_enable_irq(BSP_GPIO_IMU_INT_1);
  bsp_gpio_enable_irq(BSP_GPIO_IMU_INT_2);

  return true;
}

void ICM42688::Prase() {
  int16_t raw_x = 0, raw_y = 0, raw_z = 0;
  raw_x = static_cast<int16_t>(dma_buf[2] << 8 | dma_buf[3]);
  raw_y = static_cast<int16_t>(dma_buf[4] << 8 | dma_buf[5]);
  raw_z = static_cast<int16_t>(dma_buf[6] << 8 | dma_buf[7]);

  std::array<float, 3> accl = {static_cast<float>(raw_x),
                               static_cast<float>(raw_y),
                               static_cast<float>(raw_z)};

  for (float &it : accl) {
    it /= 2048.0f;
  }

  memset(&(this->accl_), 0, sizeof(this->accl_));

  for (int i = 0; i < 3; i++) {
    this->accl_.x += this->rot_.rot_mat[0][i] * accl[i];
    this->accl_.y += this->rot_.rot_mat[1][i] * accl[i];
    this->accl_.z += this->rot_.rot_mat[2][i] * accl[i];
  }

  raw_x = static_cast<int16_t>(dma_buf[8] << 8 | dma_buf[9]);
  raw_y = static_cast<int16_t>(dma_buf[10] << 8 | dma_buf[11]);
  raw_z = static_cast<int16_t>(dma_buf[12] << 8 | dma_buf[13]);

  std::array<float, 3> gyro = {static_cast<float>(raw_x),
                               static_cast<float>(raw_y),
                               static_cast<float>(raw_z)};

  for (float &it : gyro) {
    it = it / 16.384f * M_DEG2RAD_MULT;
  }

  memset(&(this->gyro_), 0, sizeof(this->gyro_));

  for (int i = 0; i < 3; i++) {
    this->gyro_.x += this->rot_.rot_mat[0][i] * gyro[i];
    this->gyro_.y += this->rot_.rot_mat[1][i] * gyro[i];
    this->gyro_.z += this->rot_.rot_mat[2][i] * gyro[i];
  }

  this->gyro_.x -= this->cali_.data_.gyro_offset.x;
  this->gyro_.y -= this->cali_.data_.gyro_offset.y;
  this->gyro_.z -= this->cali_.data_.gyro_offset.z;

  int16_t raw_temp = static_cast<int16_t>(dma_buf[0] << 8 | dma_buf[1]);

  this->temp_ = raw_temp;
}

bool ICM42688::StartRecv() {
  Read(0x1d, dma_buf, sizeof(dma_buf));
  return true;
}
