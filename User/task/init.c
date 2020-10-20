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
void Task_Init(void *argument) {
  (void)argument;

  /* Init robot. */
  Config_Get(&task_runtime.robot_id);

  task_runtime.robot_param = Config_GetRobotParam(task_runtime.robot_id.model);
  task_runtime.config_pilot = Config_GetPilotCfg(task_runtime.robot_id.pilot);

  osKernelLock();
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
  task_runtime.thread.referee = osThreadNew(Task_Referee, NULL, &attr_referee);
  osKernelUnlock();

  osThreadTerminate(osThreadGetId());
}
