/**
 * @file ctrl_launcher.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 发射器控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mod_launcher.h"
#include "om.h"
#include "thd.h"

#define THD_PERIOD_MS (2)
#define THD_DELAY_TICK (pdMS_TO_TICKS(THD_PERIOD_MS))

void thd_ctrl_launcher(void* arg) {
  runtime_t* runtime = arg;

  launcher_t launcher;
  cmd_launcher_t launcher_cmd;
  motor_feedback_group_t fric_motor_fb;
  motor_feedback_group_t trig_motor_fb;
  referee_for_launcher_t ref_launcher;
  motor_control_t launcher_fric_out;
  motor_control_t launcher_trig_out;
  ui_launcher_t launcher_ui;

  om_topic_t* fric_out_tp = om_config_topic(NULL, "A", "launcher_fric_out");
  om_topic_t* trig_out_tp = om_config_topic(NULL, "A", "launcher_trig_out");
  om_topic_t* ui_tp = om_config_topic(NULL, "A", "launcher_ui");
  om_topic_t* mt_fric_tp = om_find_topic("launcher_fric_motor_fb", UINT32_MAX);
  om_topic_t* mt_trig_tp = om_find_topic("launcher_trig_motor_fb", UINT32_MAX);
  om_topic_t* ref_tp = om_find_topic("referee_launcher", UINT32_MAX);
  om_topic_t* cmd_tp = om_find_topic("cmd_launcher", UINT32_MAX);

  om_suber_t* motor_fric_sub =
      om_subscript(mt_fric_tp, OM_PRASE_VAR(fric_motor_fb), NULL);
  om_suber_t* motor_trig_sub =
      om_subscript(mt_trig_tp, OM_PRASE_VAR(trig_motor_fb), NULL);
  om_suber_t* ref_sub = om_subscript(ref_tp, OM_PRASE_VAR(ref_launcher), NULL);
  om_suber_t* cmd_sub = om_subscript(cmd_tp, OM_PRASE_VAR(launcher_cmd), NULL);

  /* 初始化发射器 */
  launcher_init(&launcher, &(runtime->cfg.robot_param->launcher),
                1000.0f / (float)THD_PERIOD_MS);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取控制指令、姿态、IMU、裁判系统、电机反馈 */
    om_suber_dump(motor_fric_sub);
    om_suber_dump(motor_trig_sub);
    om_suber_dump(ref_sub);
    om_suber_dump(cmd_sub);

    vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
    launcher_update_feedback(&launcher, &trig_motor_fb, &fric_motor_fb);
    launcher_control(&launcher, &launcher_cmd, &ref_launcher,
                     xTaskGetTickCount());
    launcher_pack_output(&launcher, &launcher_trig_out, &launcher_fric_out);
    launcher_pack_ui(&launcher, &launcher_ui);
    xTaskResumeAll();

    om_publish(fric_out_tp, OM_PRASE_VAR(launcher_fric_out), true);
    om_publish(trig_out_tp, OM_PRASE_VAR(launcher_trig_out), true);
    om_publish(ui_tp, OM_PRASE_VAR(launcher_ui), true);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, THD_DELAY_TICK);
  }
}
THREAD_DECLEAR(thd_ctrl_launcher, 384, 2);
