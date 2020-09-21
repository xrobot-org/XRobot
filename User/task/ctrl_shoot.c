/*
  射击控制任务
  
  控制射击行为。
  
  从CAN总线接收底盘电机反馈，根据接收到的控制命令，控制电机输出。
*/

/* Includes ----------------------------------------------------------------- */
#include "module\robot.h"
#include "module\shoot.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t *can;

#ifdef DEBUG
CMD_Shoot_Ctrl_t shoot_ctrl;
Shoot_t shoot;
#else
static CMD_Shoot_Ctrl_t shoot_ctrl;
static Shoot_t shoot;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_CtrlShoot(void *argument) {
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_CTRL_SHOOT;
  Task_Param_t *task_param = (Task_Param_t *)argument;
  /* Task Setup */
  osDelay(TASK_INIT_DELAY_CTRL_SHOOT);

  while ((can = CAN_GetDevice()) == NULL) {
    osDelay(delay_tick);
  }

  Shoot_Init(&shoot, &(task_param->config_robot->param.shoot),
             (float)TASK_FREQ_HZ_CTRL_SHOOT);

  uint32_t tick = osKernelGetTickCount();
  uint32_t wakeup = HAL_GetTick();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.ctrl_shoot = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    const uint32_t flag = SIGNAL_CAN_MOTOR_RECV;
    if (osThreadFlagsWait(flag, osFlagsWaitAll, delay_tick) != flag) {
      CAN_Motor_ControlShoot(0.0f, 0.0f, 0.0f);
      
    } else {
      osMessageQueueGet(task_param->msgq.cmd.shoot, &shoot_ctrl, NULL, 0);

      osKernelLock();
      const uint32_t now = HAL_GetTick();
      Shoot_UpdateFeedback(&shoot, can);
      Shoot_Control(&shoot, &shoot_ctrl, (float)(now - wakeup)/1000.0f);
      wakeup = now;
      CAN_Motor_ControlShoot(shoot.out[SHOOT_ACTR_FRIC1_IDX],
                             shoot.out[SHOOT_ACTR_FRIC2_IDX],
                             shoot.out[SHOOT_ACTR_TRIG_IDX]);
      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
