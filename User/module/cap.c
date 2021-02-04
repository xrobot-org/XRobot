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
#define CAP_CUTOFF_VOLT 12.0f

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

void Cap_Control(CAN_Capacitor_t *cap, const Referee_t *referee,
                 CAN_CapOutput_t *cap_out) {
  if (referee->ref_status != REF_STATUS_RUNNING) {
    /* 当裁判系统离线时，依然使用裁判系统进程传来的数据 */
    cap_out->power_limit = referee->robot_status.chassis_power_limit;
  } else {
    /* 当裁判系统在线时，使用算法控制裁判系统输出（即电容输入） */
    cap_out->power_limit =
        PowerLimit_CapInput(referee->power_heat.chassis_watt,
                            referee->robot_status.chassis_power_limit,
                            referee->power_heat.chassis_pwr_buff);
  }
  cap->cap_status = CAN_CAP_STATUS_RUNNING;
  cap->percentage = Capacity_GetCapacitorRemain(cap->cap_feedback.cap_volt,
                                                cap->cap_feedback.input_volt,
                                                CAP_CUTOFF_VOLT);
}
