/**
 * @file cap.c
 * @author cndabai
 * @brief 电容模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "cap.h"

#include "component/capacity.h"
#include "component/limiter.h"
#include "device/can.h"
#include "device/referee.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */

#define CAP_CUTOFF_VOLT 12.0f /* 电容截止电压，要高于电调最低工作电压 */

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

bool Cap_RefIsReady(const Referee_ForCap_t *referee) {
  return (referee->chassis_power_limit > 0.0f) &&
         (referee->chassis_pwr_buff > 0.0f) && (referee->chassis_watt > 0.0f);
}

void Cap_Update(Cap_t *cap, const CAN_CapFeedback_t *cap_fb) {
  /* 更新电容状态和百分比 */
  cap->cap_status = CAN_CAP_STATUS_RUNNING;
  cap->percentage = Capacity_GetCapacitorRemain(
      cap_fb->cap_volt, cap_fb->input_volt, CAP_CUTOFF_VOLT);
}

/**
 * @brief 运行电容控制逻辑
 *
 * @param cap 电容数据结构体
 * @param referee 裁判系统数据
 * @param cap_out 电容输出结构体
 */
void Cap_Control(const Referee_ForCap_t *referee, CAN_CapOutput_t *cap_out) {
  if (referee->status != REF_STATUS_RUNNING || !Cap_RefIsReady(referee)) {
    /* 当裁判系统离线时，依然使用裁判系统进程传来的数据 */
    cap_out->power_limit = referee->chassis_power_limit;
  } else {
    /* 当裁判系统在线时，使用算法控制裁判系统输出（即电容输入） */
    cap_out->power_limit =
        PowerLimit_CapInput(referee->chassis_watt, referee->chassis_power_limit,
                            referee->chassis_pwr_buff);
  }
}

/**
 * @brief 电容模块离线时的控制逻辑
 *
 * @param cap 电容数据结构体
 * @param cap_out 电容输出结构体
 * @param power_chassis 底盘功率
 */
void Cap_HandleOffline(Cap_t *cap, CAN_CapOutput_t *cap_out,
                       float power_chassis) {
  cap->cap_status = CAN_CAP_STATUS_OFFLINE;
  cap_out->power_limit = power_chassis;
}

/**
 * @brief 导出电容数据
 *
 * @param cap 电容数据
 * @param ui 结构体
 */
void Cap_PackUi(const Cap_t *cap, UI_CapUI_t *ui) {
  ui->percentage = cap->percentage;
  ui->online = (cap->cap_status == CAN_CAP_STATUS_RUNNING);
}
