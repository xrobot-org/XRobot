/*
  初始化任务

  根据机器人的FLASH配置，决定生成哪些任务。
*/

/* Includes ----------------------------------------------------------------- */
#include "bsp\flash.h"
#include "bsp\usb.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/*!
 * \brief 初始化
 *
 * \param argument 未使用
 */
void Task_Init(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  Config_Get(&task_runtime.robot_cfg); /* 获取机器人配置 */

  /* 获取机器人参数和操作手信息 */
  task_runtime.robot_param = Config_GetRobotParam(task_runtime.robot_cfg.model);
  task_runtime.config_pilot = Config_GetPilotCfg(task_runtime.robot_cfg.pilot);

  osKernelLock();
  /* 创建任务 */
  task_runtime.thread.atti_esti =
      osThreadNew(Task_AttiEsti, NULL, &attr_atti_esti);
  task_runtime.thread.cli = osThreadNew(Task_CLI, NULL, &attr_cli);
  task_runtime.thread.command = osThreadNew(Task_Command, NULL, &attr_command);
  task_runtime.thread.ctrl_chassis =
      osThreadNew(Task_CtrlChassis, NULL, &attr_ctrl_chassis);
  task_runtime.thread.ctrl_gimbal =
      osThreadNew(Task_CtrlGimbal, NULL, &attr_ctrl_gimbal);
  task_runtime.thread.ctrl_shoot =
      osThreadNew(Task_CtrlShoot, NULL, &attr_ctrl_shoot);
  task_runtime.thread.info = osThreadNew(Task_Info, NULL, &attr_info);
  task_runtime.thread.monitor = osThreadNew(Task_Monitor, NULL, &attr_monitor);
  task_runtime.thread.motor = osThreadNew(Task_Motor, NULL, &attr_motor);
  task_runtime.thread.referee = osThreadNew(Task_Referee, NULL, &attr_referee);
  osKernelUnlock();

  osThreadTerminate(osThreadGetId()); /* 结束自身 */
}
