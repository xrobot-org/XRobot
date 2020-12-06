/*
 * 电容模组
 */

/* Includes ----------------------------------------------------------------- */
#include "cap.h"

#include "component\capacity.h"
#include "component\limiter.h"
#include "device\can.h"
#include "device\referee.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
#define CHASSIS_POWER_MAX_WITHOUT_REF 80.0f
#define CAP_STATUS_THRESHOLD_PERCENTAGE_CHARGE 0.1f
#define CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING1 0.15f
#define CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING2 0.3f
#define CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING3 0.5f
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

void Cap_Control(CAN_Capacitor_t *cap, const Referee_t *referee,
                 CAN_CapOutput_t *cap_out) {
  if (referee->ref_status != REF_STATUS_RUNNING) {
    cap_out->power_limit = CHASSIS_POWER_MAX_WITHOUT_REF;
  } else {
    cap_out->power_limit =
        PowerLimit_CapInput(referee->power_heat.chassis_watt,
                            referee->robot_status.chassis_power_limit,
                            referee->power_heat.chassis_pwr_buff);
  }

  float percentage = Capacity_GetBatteryRemain(cap->cap_feedback.cap_volt);
  if (percentage < CAP_STATUS_THRESHOLD_PERCENTAGE_CHARGE) {
    cap->cap_status = CAP_STATUS_CHARGING;
  } else if (percentage < CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING1) {
    cap->cap_status = CAP_STATUS_RUNNING1;
  } else if (percentage < CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING2) {
    cap->cap_status = CAP_STATUS_RUNNING2;
  } else if (percentage < CAP_STATUS_THRESHOLD_PERCENTAGE_RUNNING3) {
    cap->cap_status = CAP_STATUS_RUNNING3;
  }
}
