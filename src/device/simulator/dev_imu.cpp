#include "dev_imu.hpp"

#include <comp_utils.hpp>

#include "bsp_time.h"
#include "webots/accelerometer.h"
#include "webots/gyro.h"
#include "webots/inertial_unit.h"

using namespace Device;

IMU::IMU(IMU::Param& param)
    : accl_tp_((param.tp_name_prefix + std::string("_accl")).c_str()),
      gyro_tp_((param.tp_name_prefix + std::string("_gyro")).c_str()),
      eulr_tp_((param.tp_name_prefix + std::string("_eulr")).c_str()),
      ahrs_handle_(wb_robot_get_device("imu")),
      gyro_handle_(wb_robot_get_device("gyro")),
      accl_handle_(wb_robot_get_device("accl")),
      cmd_(this, IMU::ShowCMD, "imu", System::Term::DevDir()) {
  wb_inertial_unit_enable(this->ahrs_handle_, 1);
  wb_accelerometer_enable(this->accl_handle_, 1);
  wb_gyro_enable(this->gyro_handle_, 1);

  auto thread_fn = [](IMU* imu) {
    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      imu->accl_tp_.Publish(imu->accl_);
      imu->gyro_tp_.Publish(imu->gyro_);
      imu->eulr_tp_.Publish(imu->eulr_);

      imu->Update();

      imu->thread_.SleepUntil(1, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, this, "imu", 512, System::Thread::REALTIME);
}

void IMU::Update() {
  const double* accl_data = wb_accelerometer_get_values(this->accl_handle_);
  const double* gyro_data = wb_gyro_get_values(this->gyro_handle_);
  const double* eulr_data =
      wb_inertial_unit_get_roll_pitch_yaw(this->ahrs_handle_);

  this->eulr_.rol = static_cast<float>(eulr_data[1]);
  this->eulr_.pit = static_cast<float>(eulr_data[0]);
  this->eulr_.yaw = static_cast<float>(eulr_data[2]);

  if (this->eulr_.rol < 0.0f) {
    this->eulr_.rol += M_2PI;
  }

  if (this->eulr_.pit < 0.0f) {
    this->eulr_.pit += M_2PI;
  }

  if (this->eulr_.yaw < 0.0f) {
    this->eulr_.yaw += M_2PI;
  }

  this->gyro_.x = static_cast<float>(gyro_data[0]);
  this->gyro_.y = static_cast<float>(gyro_data[1]);
  this->gyro_.z = static_cast<float>(gyro_data[2]);

  this->accl_.x = static_cast<float>(accl_data[0]);
  this->accl_.y = static_cast<float>(accl_data[1]);
  this->accl_.z = static_cast<float>(accl_data[2]);
}

int IMU::ShowCMD(IMU* imu, int argc, char** argv) {
  if (argc == 1) {
    printf("[show] [time] [delay] 在time时间内每隔delay打印一次数据\r\n");
  } else if (argc == 4) {
    int time = std::stoi(argv[2]);
    int delay = std::stoi(argv[3]);

    if (delay > 1000) {
      delay = 1000;
    }

    if (delay < 2) {
      delay = 2;
    }

    while (time > delay) {
      printf(
          "Accl[x:%f y:%f z:%f] Gyro[x:%f y:%f z:%f] Eulr[pit:%f rol:%f "
          "yaw:%f]\r\n",
          imu->accl_.x, imu->accl_.y, imu->accl_.z, imu->gyro_.x, imu->gyro_.y,
          imu->gyro_.z, imu->eulr_.pit.Value(), imu->eulr_.rol.Value(),
          imu->eulr_.yaw.Value());
      System::Thread::Sleep(delay);
      ms_clear_line();
      time -= delay;
    }
  }

  return 0;
}
