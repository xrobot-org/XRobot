/*
        用于初始化所有任务的任务

*/

/* Includes ------------------------------------------------------------------*/
#include "bsp\flash.h"
#include "bsp\usb.h"
#include "task\user_task.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char *const ROBOT_ID_MEAASGE =
    " -------------------------------------------------------------------\r\n"
    " Robot Model: %s\tRobot Pilot: %s \r\n"
    " -------------------------------------------------------------------\r\n"
    "\r\n";

Task_Param_t task_param;  // TODO: Add static when release

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Init(void *argument) {
  (void)argument;

  /* Init robot. */
  Robot_GetRobotID(&task_param.robot_id);

  task_param.config_robot = Robot_GetConfig(task_param.robot_id.model);
  task_param.config_pilot = Robot_GetPilotConfig(task_param.robot_id.pilot);

  /* Command Line Interface. */
  BSP_USB_Printf(ROBOT_ID_MEAASGE,
                 Robot_GetNameByModel(task_param.robot_id.model),
                 Robot_GetNameByPilot(task_param.robot_id.pilot));

  osKernelLock();
  task_param.thread.atti_esti =
      osThreadNew(Task_AttiEsti, &task_param, &attr_atti_esti);
  task_param.thread.cli = osThreadNew(Task_CLI, &task_param, &attr_cli);
  task_param.thread.command =
      osThreadNew(Task_Command, &task_param, &attr_command);
  task_param.thread.ctrl_chassis =
      osThreadNew(Task_CtrlChassis, &task_param, &attr_ctrl_chassis);
  task_param.thread.ctrl_gimbal =
      osThreadNew(Task_CtrlGimbal, &task_param, &attr_ctrl_gimbal);
  //task_param.thread.ctrl_shoot =
    //  osThreadNew(Task_CtrlShoot, &task_param, &attr_ctrl_shoot);
  task_param.thread.info = osThreadNew(Task_Info, &task_param, &attr_info);
  task_param.thread.monitor =
      osThreadNew(Task_Monitor, &task_param, &attr_monitor);
  task_param.thread.referee =
      osThreadNew(Task_Referee, &task_param, &attr_referee);
  osKernelUnlock();

  osThreadTerminate(osThreadGetId());
}
