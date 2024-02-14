#include "dev_mmc5603.hpp"

#include "bsp_i2c.h"
#include "bsp_time.h"

using namespace Device;

static uint8_t dma_buff[10];

#define MATRIX_SIZE (6)

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
      System::Thread::Sleep(20);
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

  this->thread_.Create(thread_fn, this, "mmc5603_thread", 4096,
                       System::Thread::REALTIME);
}

bool MMC5603::Init() {
  /* Check Product id */

  if (uint8_t product_id =
          bsp_i2c_mem_read_byte(BSP_I2C_MAGN, 0x30, 0x39) != 0x10) {
    OMLOG_ERROR("mmc5603 product id read error:%d", product_id);

    return false;
  }

  /* Reset */
  bsp_i2c_mem_write_byte(BSP_I2C_MAGN, 0x30, 0x1B, 0x10);

  /* Set CTRL_1 bandwith */
  bsp_i2c_mem_write_byte(BSP_I2C_MAGN, 0x30, 0x1C, 0x02);
  /* Set ODR sampling_rate */
  bsp_i2c_mem_write_byte(BSP_I2C_MAGN, 0x30, 0x1A, 0xff);
  /* Set CTRL_0 Auto_SR_en Cmm_freq_en */
  bsp_i2c_mem_write_byte(BSP_I2C_MAGN, 0x30, 0x1B, 0xa0);
  /* Set CTRL_2 Cmm_en */
  bsp_i2c_mem_write_byte(BSP_I2C_MAGN, 0x30, 0x1D, 0x10);

  return true;
}

void MMC5603::StartRecv() {
  bsp_i2c_mem_read(BSP_I2C_MAGN, 0x30, 0x00, dma_buff, sizeof(dma_buff), false);
}

void MMC5603::PraseData() {
  int32_t raw[3];
  raw[0] = dma_buff[6] | (dma_buff[1] << 4) | (dma_buff[0] << 12);
  raw[1] = dma_buff[7] | (dma_buff[3] << 4) | (dma_buff[2] << 12);
  raw[2] = dma_buff[8] | (dma_buff[5] << 4) | (dma_buff[4] << 12);

  for (int32_t &i : raw) {
    i -= (1 << 19);
  }

  float data[3];

  data[0] = static_cast<float>(raw[0]) * 0.0625f * 0.001f;
  data[1] = static_cast<float>(raw[1]) * 0.0625f * 0.001f;
  data[2] = static_cast<float>(raw[2]) * 0.0625f * 0.001f;

  float x = 0, y = 0, z = 0;

  for (int i = 0; i < 3; i++) {
    x += this->rot_.rot_mat[0][i] * data[i];
    y += this->rot_.rot_mat[1][i] * data[i];
    z += this->rot_.rot_mat[2][i] * data[i];
  }

  this->raw_magn_.x = x;
  this->raw_magn_.y = y;
  this->raw_magn_.z = z;

  this->magn_.x = (this->raw_magn_.x - cali_data_.data_.offset.x) /
                  cali_data_.data_.scale.x;
  this->magn_.y = (this->raw_magn_.y - cali_data_.data_.offset.y) /
                  cali_data_.data_.scale.y;
  this->magn_.z = (this->raw_magn_.z - cali_data_.data_.offset.z) /
                  cali_data_.data_.scale.z;

  this->intensity_ =
      sqrtf(magn_.x * magn_.x + magn_.y * magn_.y + magn_.z * magn_.z);

  if (intensity_ > 1.06f || intensity_ < 0.94f) {
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
      /* Reference: https://zhuanlan.zhihu.com/p/37265316 */

      /* 系数矩阵 */
      auto m_matrix = new std::array<float, MATRIX_SIZE *(MATRIX_SIZE + 1)>;
      /* 方程组的解对应最小二乘椭球拟合中的，a，b，c，d，e，f */
      std::array<float, MATRIX_SIZE> solve{};
      int N = 0;

      for (auto &item : *m_matrix) {
        item = 0;
      }

      printf("开始校准，请尽量旋转磁力计到每一个可能的角度\r\n");

      System::Thread::Sleep(2000);

      auto prase_input = [&](float x, float y, float z) {
        float V[MATRIX_SIZE + 1];
        N++;
        V[0] = y * y;
        V[1] = z * z;
        V[2] = x;
        V[3] = y;
        V[4] = z;
        V[5] = 1.0;
        V[6] = -x * x;
        // 构建系数矩阵，并进行累加
        for (uint8_t row = 0; row < MATRIX_SIZE; row++) {
          for (uint8_t column = 0; column < MATRIX_SIZE + 1; column++) {
            (*m_matrix)[row * (MATRIX_SIZE + 1) + column] += V[row] * V[column];
          }
        }
      };

      for (int i = 0; i < 3000; i++) {
        prase_input(mmc5603->raw_magn_.x, mmc5603->raw_magn_.y,
                    mmc5603->raw_magn_.z);
        printf("进度：%d/3000", i);
        System::Thread::Sleep(10);
        ms_clear_line();
      }

      printf("\r\nStart data prase.\r\n");

      for (auto &item : *m_matrix) {
        item /= static_cast<float>(N);
      }

      auto row2_add_k_row1 = [&](float k, int row1, int row2) {
        for (uint8_t column = 0; column < MATRIX_SIZE + 1; column++) {
          (*m_matrix)[row2 * (MATRIX_SIZE + 1) + column] +=
              k * (*m_matrix)[row1 * (MATRIX_SIZE + 1) + column];
        }
      };

      auto row2_swop_row1 = [&](int row1, int row2) {
        float tmp = 0;
        for (uint8_t column = 0; column < MATRIX_SIZE + 1; column++) {
          tmp = (*m_matrix)[row1 * (MATRIX_SIZE + 1) + column];
          (*m_matrix)[row1 * (MATRIX_SIZE + 1) + column] =
              (*m_matrix)[row2 * (MATRIX_SIZE + 1) + column];
          (*m_matrix)[row2 * (MATRIX_SIZE + 1) + column] = tmp;
        }
      };

      auto move_biggest_element_to_top = [&](int k) {
        int row = 0;

        for (row = k + 1; row < MATRIX_SIZE; row++) {
          if (fabs((*m_matrix)[k * (MATRIX_SIZE + 1) + k]) <
              fabs((*m_matrix)[row * (MATRIX_SIZE + 1) + k])) {
            row2_swop_row1(k, row);
          }
        }
      };

      float k = 0;
      /* 进行第k次的运算，主要是针对k行以下的行数把k列的元素都变成0 */
      for (uint8_t cnt = 0; cnt < MATRIX_SIZE; cnt++) {
        // 把k行依据k列的元素大小，进行排序
        move_biggest_element_to_top(cnt);
        if ((*m_matrix)[cnt * (MATRIX_SIZE + 1) + cnt] == 0) {
          return -1;  // 返回值表示错误
        }
        // 把k行下面的行元素全部消成0，整行变化
        for (uint8_t row = cnt + 1; row < MATRIX_SIZE; row++) {
          k = -(*m_matrix)[row * (MATRIX_SIZE + 1) + cnt] /
              (*m_matrix)[cnt * (MATRIX_SIZE + 1) + cnt];
          row2_add_k_row1(k, cnt, row);
        }
      }
      k = 0;
      for (uint8_t row = 0; row < MATRIX_SIZE; row++) {
        k = 1 / (*m_matrix)[row * (MATRIX_SIZE + 1) + row];
        for (uint8_t column = 0; column < MATRIX_SIZE + 1; column++) {
          (*m_matrix)[row * (MATRIX_SIZE + 1) + column] *= k;
        }
      }

      int16_t row = MATRIX_SIZE - 1;

      for (; row >= 0; row--) {
        solve[row] = (*m_matrix)[row * (MATRIX_SIZE + 1) + MATRIX_SIZE];
        for (uint8_t column = MATRIX_SIZE - 1; column >= row + 1; column--) {
          solve[row] -=
              (*m_matrix)[row * (MATRIX_SIZE + 1) + column] * solve[column];
        }
      }

      printf("  a = %f| b = %f| c = %f| d = %f| e = %f| f = %f ", solve[0],
             solve[1], solve[2], solve[3], solve[4], solve[5]);
      printf("\r\n");
      printf("\r\n");

      float &a = solve[0];
      float &b = solve[1];
      float &c = solve[2];
      float &d = solve[3];
      float &e = solve[4];
      float &f = solve[5];

      float X0 = 0, Y0 = 0, Z0 = 0, A = 0, B = 0, C = 0;
      X0 = -c / 2;
      Y0 = -d / (2 * a);
      Z0 = -e / (2 * b);
      A = sqrt(X0 * X0 + a * Y0 * Y0 + b * Z0 * Z0 - f);
      B = A / sqrt(a);
      C = A / sqrt(b);
      printf("\r\n");
      printf("  X0 = %f| Y0 = %f| Z0 = %f| A = %f| B = %f| C = %f \r\n", X0, Y0,
             Z0, A, B, C);

      mmc5603->cali_data_.data_.offset.x = X0;
      mmc5603->cali_data_.data_.offset.y = Y0;
      mmc5603->cali_data_.data_.offset.z = Z0;
      mmc5603->cali_data_.data_.scale.x = A;
      mmc5603->cali_data_.data_.scale.y = B;
      mmc5603->cali_data_.data_.scale.z = C;

      mmc5603->cali_data_.Set();

      delete m_matrix;
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
        printf("x:%+5f y:%+5f z:%+5f intensity:%f", mmc5603->raw_magn_.x,
               mmc5603->raw_magn_.y, mmc5603->raw_magn_.z, mmc5603->intensity_);
        System::Thread::Sleep(delay);
        ms_clear_line();
        time -= delay;
      } while (time > delay);

      printf("x:%+5f y:%+5f z:%+5f intensity:%f", mmc5603->raw_magn_.x,
             mmc5603->raw_magn_.y, mmc5603->raw_magn_.z, mmc5603->intensity_);

      printf("\r\n");
    }
  } else {
    printf("参数错误\r\n");
    return -1;
  }

  return 0;
}
