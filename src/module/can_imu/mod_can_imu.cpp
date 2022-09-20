#include "mod_can_imu.hpp"

using namespace Module;

static uint8_t imu_tx_buff[8];

static const uint8_t IMU_DEVICE_ID = 0x01;

static const uint8_t ACCL_DATA_ID = 0x01;
static const uint8_t GYRO_DATA_ID = 0x02;
static const uint8_t EULR_DATA_ID = 0x03;

CanIMU::CanIMU() {
  auto imu_thread = [](void *arg) {
    CanIMU *imu = static_cast<CanIMU *>(arg);

    DECLARE_SUBER(eulr_, imu->eulr_, "imu_eulr");
    DECLARE_SUBER(gyro_, imu->gyro_, "imu_gyro");
    DECLARE_SUBER(accl_, imu->accl_, "imu_accl");

    while (1) {
#if IMU_SEND_ACCL
      accl_.DumpData();
      imu->SendAccl();
#endif

#if IMU_SEND_GYRO
      gyro_.DumpData();
      imu->SendGyro();
#endif

#if IMU_SEND_EULR
      eulr_.DumpData();
      imu->SendEulr();
#endif

      imu->thread_.Sleep(1000 / IMU_SEND_FREQ);
    }
  };

  THREAD_DECLEAR(this->thread_, imu_thread, 512, System::Thread::Medium, this);
}

void CanIMU::SendAccl() {
  int16_t *tmp = (int16_t *)imu_tx_buff;
  tmp[1] = this->accl_.x / 6.0f * (float)INT16_MAX;
  tmp[2] = this->accl_.y / 6.0f * (float)INT16_MAX;
  tmp[3] = this->accl_.z / 6.0f * (float)INT16_MAX;
  imu_tx_buff[0] = IMU_DEVICE_ID;
  imu_tx_buff[1] = ACCL_DATA_ID;
  bsp_can_trans_packet(BSP_CAN_1, IMU_SEND_CAN_ID, imu_tx_buff, &this->mailbox_,
                       1);
}

void CanIMU::SendGyro() {
  int16_t *tmp = (int16_t *)imu_tx_buff;
  tmp[1] = this->gyro_.x / 20.0f * (float)INT16_MAX;
  tmp[2] = this->gyro_.y / 20.0f * (float)INT16_MAX;
  tmp[3] = this->gyro_.z / 20.0f * (float)INT16_MAX;
  imu_tx_buff[0] = IMU_DEVICE_ID;
  imu_tx_buff[1] = GYRO_DATA_ID;
  bsp_can_trans_packet(BSP_CAN_1, IMU_SEND_CAN_ID, imu_tx_buff, &this->mailbox_,
                       1);
}

void CanIMU::SendEulr() {
  int16_t *tmp = (int16_t *)imu_tx_buff;
  tmp[1] = this->eulr_.pit / M_2PI * (float)INT16_MAX;
  tmp[2] = this->eulr_.rol / M_2PI * (float)INT16_MAX;
  tmp[3] = this->eulr_.yaw / M_2PI * (float)INT16_MAX;
  imu_tx_buff[0] = IMU_DEVICE_ID;
  imu_tx_buff[1] = EULR_DATA_ID;
  bsp_can_trans_packet(BSP_CAN_1, IMU_SEND_CAN_ID, imu_tx_buff, &this->mailbox_,
                       1);
}
