#include "dev_mmc5603.hpp"

#include <cmath>
#include <cstring>
#include <thread.hpp>

#include "bsp_i2c.h"
#include "bsp_time.h"
#include "om_log.h"

using namespace Device;

static uint8_t data[1], dma_buff[10];

static uint8_t mmc5603_reset = 0x10;
static uint8_t mmc5603_odr = 0xff;
static uint8_t mmc5603_ctrl_0 = 0xa0;
static uint8_t mmc5603_ctrl_2 = 0x98;

MMC5603::MMC5603(MMC5603::Rotation &rot)
    : rot_(rot),
      magn_tp_("magn"),
      cmd_(this, CaliCMD, "mmc5603"),
      cali_data_("mmc5603_cali"),
      raw_(0) {
  auto recv_cplt_callback = [](void *arg) {
    MMC5603 *mmc5603 = static_cast<MMC5603 *>(arg);
    mmc5603->raw_.Post();
  };

  bsp_i2c_register_callback(BSP_I2C_MAGN, BSP_I2C_RX_CPLT_CB,
                            recv_cplt_callback, this);

  auto thread_fn = [](MMC5603 *mmc5603) {
    while (!mmc5603->Init()) {
      System::Thread::Sleep(1);
    }

    uint32_t last_wakeup_time = bsp_time_get_ms();

    while (1) {
      mmc5603->StartRecv();
      if (mmc5603->raw_.Wait(20)) {
        mmc5603->PraseData();
        mmc5603->magn_tp_.Publish(mmc5603->magn_);
      } else {
        OMLOG_ERROR("mmc5603 recv timeout");
      }

      mmc5603->thread_.SleepUntil(1, last_wakeup_time);
    }
  };

  this->thread_.Create(thread_fn, this, "mmc5603_thread", 512,
                       System::Thread::REALTIME);
}

bool MMC5603::Init() {
  /* Check Product id */
  bsp_i2c_mem_read(BSP_I2C_MAGN, 0x60, 0x39, data, 1, true);
  if (*data != 0x10) {
    return false;
  }

  /* Reset */
  bsp_i2c_mem_write(BSP_I2C_MAGN, 0x60, 0x1B, &mmc5603_reset, 1, true);

  /* Set ODR */
  bsp_i2c_mem_write(BSP_I2C_MAGN, 0x60, 0x1A, &mmc5603_odr, 1, true);
  bsp_i2c_mem_write(BSP_I2C_MAGN, 0x60, 0x1B, &mmc5603_ctrl_0, 1, true);
  bsp_i2c_mem_write(BSP_I2C_MAGN, 0x60, 0x1D, &mmc5603_ctrl_2, 1, true);

  return true;
}

void MMC5603::StartRecv() {
  bsp_i2c_mem_read(BSP_I2C_MAGN, 0x60, 0x00, dma_buff, sizeof(dma_buff), false);
}

void MMC5603::PraseData() {
  int32_t raw[3];
  raw[0] = dma_buff[6] | (dma_buff[1] << 4) | (dma_buff[0] << 12);
  raw[1] = dma_buff[7] | (dma_buff[3] << 4) | (dma_buff[2] << 12);
  raw[2] = dma_buff[8] | (dma_buff[5] << 4) | (dma_buff[4] << 12);

  for (int &i : raw) {
    i -= (1 << 19);
  }

  float data[3];

  data[0] = static_cast<float>(raw[0]) * 0.0625f * 0.001f;
  data[1] = static_cast<float>(raw[1]) * 0.0625f * 0.001f;
  data[2] = static_cast<float>(raw[2]) * 0.0625f * 0.001f;

  memset(&raw_magn_, 0, sizeof(raw_magn_));

  for (int i = 0; i < 3; i++) {
    this->raw_magn_.x += this->rot_.rot_mat[0][i] * data[i];
    this->raw_magn_.y += this->rot_.rot_mat[1][i] * data[i];
    this->raw_magn_.z += this->rot_.rot_mat[2][i] * data[i];
  }

  this->magn_.x = (this->raw_magn_.x - cali_data_.data_.offset.x) /
                  cali_data_.data_.scale.x;
  this->magn_.y = (this->raw_magn_.y - cali_data_.data_.offset.y) /
                  cali_data_.data_.scale.y;
  this->magn_.z = (this->raw_magn_.z - cali_data_.data_.offset.z) /
                  cali_data_.data_.scale.z;

  this->intensity_ =
      sqrtf(magn_.x * magn_.x + magn_.y * magn_.y + magn_.z * magn_.z);

  if (intensity_ > 0.55f || intensity_ < 0.45f) {
    magn_.x = magn_.y = magn_.z = 0.0f;
  }
}

int MMC5603::CaliCMD(MMC5603 *mmc5603, int argc, char **argv) {
  if (argc == 1) {
    printf("show [time] [delay] 在time时间内每隔delay打印一次数据\r\n");
    printf("list 列出校准数据\r\n");
    printf("cali 开始校准\r\n");
  } else if (argc == 2) {
    if (strcmp(argv[1], "list") == 0) {
      printf(
          "校准数据 "
          ":\r\noffset\r\n\tx:%f\r\n\ty:%f\r\n\tz:%f\r\nscale\r\n\tx:%"
          "f\r\n\ty:%f\r\n\tz:%f\r\n",
          mmc5603->cali_data_.data_.offset.x,
          mmc5603->cali_data_.data_.offset.y,
          mmc5603->cali_data_.data_.offset.z, mmc5603->cali_data_.data_.scale.x,
          mmc5603->cali_data_.data_.scale.y, mmc5603->cali_data_.data_.scale.z);
    } else if (strcmp(argv[1], "cali") == 0) {
      printf("开始校准，请尽量旋转磁力计到每一个可能的角度\r\n");
      System::Thread::Sleep(10);
      float tmp[3][2] = {{mmc5603->raw_magn_.x, mmc5603->raw_magn_.x},
                         {mmc5603->raw_magn_.y, mmc5603->raw_magn_.y},
                         {mmc5603->raw_magn_.z, mmc5603->raw_magn_.z}};
      float *data = &mmc5603->raw_magn_.x;
      for (int i = 0; i < 10000; i++) {
        for (int i = 0; i < 3; i++) {
          if (data[i] > tmp[i][0]) {
            tmp[i][0] = data[i];
          }
          if (data[i] < tmp[i][1]) {
            tmp[i][1] = data[i];
          }
        }
        printf("进度：%d/10000 %.3f %.3f %.3f", i, tmp[0][0] - tmp[0][1],
               tmp[1][0] - tmp[1][1], tmp[2][0] - tmp[2][1]);
        System::Thread::Sleep(1);
        ms_clear_line();
      }

      float *cali_data = &mmc5603->cali_data_.data_.offset.x;

      for (int i = 0; i < 3; i++) {
        cali_data[i] = (tmp[i][0] + tmp[i][1]) / 2.0f;
      }

      cali_data = &mmc5603->cali_data_.data_.scale.x;

      for (int i = 0; i < 3; i++) {
        cali_data[i] = (tmp[i][0] - tmp[i][1]);
      }

      printf(
          "校准数据 "
          ":\r\noffset\r\n\tx:%f\r\n\ty:%f\r\n\tz:%f\r\nscale\r\n\tx:%"
          "f\r\n\ty:%f\r\n\tz:%f\r\n",
          mmc5603->cali_data_.data_.offset.x,
          mmc5603->cali_data_.data_.offset.y,
          mmc5603->cali_data_.data_.offset.z, mmc5603->cali_data_.data_.scale.x,
          mmc5603->cali_data_.data_.scale.y, mmc5603->cali_data_.data_.scale.z);

      mmc5603->cali_data_.Set();

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
        printf("x:%+5f y:%+5f z:%+5f", mmc5603->raw_magn_.x,
               mmc5603->raw_magn_.y, mmc5603->raw_magn_.z);
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
