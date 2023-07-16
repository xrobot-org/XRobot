#include "dev_cap.hpp"

#include "bsp_time.h"
#include "dev_referee.hpp"

#define CAP_RES (100.0f) /* 电容数据分辨率 */

using namespace Device;

Cap::Cap(Cap::Param &param) : info_tp_("cap_info") {
  XB_UNUSED(param);
  info_ = {
      .input_volt_ = 25.0f,
      .cap_volt_ = 23.0f,
      .input_curr_ = 0.4f,
      .target_power_ = 100.0f,
      .percentage_ = 1.0f,
      .online_ = false,
  };

  info_tp_.Publish(info_);
}
