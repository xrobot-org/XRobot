/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "tim.h"

#include "task_common.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
Task_Param_t task_param;

/* TIM7 are used to generater high freq tick for debug. */
volatile unsigned long high_freq_timer_ticks;

static const osThreadAttr_t cli_attr = {
  .name = "cli",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 512
};

static const osThreadAttr_t command_attr = {
  .name = "command",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 256
};

static const osThreadAttr_t ctrl_chassis_attr = {
  .name = "ctrl_chassis",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 512
};

static const osThreadAttr_t ctrl_gimbal_attr = {
  .name = "ctrl_gimbal",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 512
};

static const osThreadAttr_t ctrl_shoot_attr = {
  .name = "ctrl_shoot",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 512
};

static const osThreadAttr_t info_attr = {
  .name = "info",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 512
};

static const osThreadAttr_t monitor_attr = {
  .name = "monitor",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256
};

static const osThreadAttr_t pos_esti_attr = {
  .name = "pos_esti",
  .priority = (osPriority_t) osPriorityRealtime,
  .stack_size = 1024
};

static const osThreadAttr_t referee_attr = {
  .name = "referee",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
/* Code inside this function should be simple and small. */
void configureTimerForRunTimeStats(void)
{
	high_freq_timer_ticks = 0;
	HAL_TIM_Base_Start_IT(&htim7);
}

/* High freq timer ticks for runtime stats */
unsigned long getRunTimeCounterValue(void)
{
	return high_freq_timer_ticks;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	task_param.thread.cli			= osThreadNew(Task_CLI,			&task_param, &cli_attr);
	task_param.thread.command		= osThreadNew(Task_Command,		&task_param, &command_attr);
	task_param.thread.ctrl_chassis	= osThreadNew(Task_CtrlChassis,	&task_param, &ctrl_chassis_attr);
	task_param.thread.ctrl_gimbal	= osThreadNew(Task_CtrlGimbal,	&task_param, &ctrl_gimbal_attr);
	task_param.thread.ctrl_shoot	= osThreadNew(Task_CtrlShoot,	&task_param, &ctrl_shoot_attr);
	task_param.thread.info			= osThreadNew(Task_Info,		&task_param, &info_attr);
	task_param.thread.monitor		= osThreadNew(Task_Monitor,		&task_param, &monitor_attr);
	task_param.thread.pos_esti		= osThreadNew(Task_PosEsti,		&task_param, &pos_esti_attr);
	task_param.thread.referee		= osThreadNew(Task_Referee,		&task_param, &referee_attr);
	
	
	#if defined ROBOT_MODEL_INFANTRY
		
	#elif defined ROBOT_MODEL_HERO
		
	#elif defined ROBOT_MODEL_ENGINEER
		
	#elif defined ROBOT_MODEL_DRONE
		
	#elif defined ROBOT_MODEL_SENTRY

	#else
		
		#error: Must define ROBOT_MODEL_XXXX.
		
	#endif
		
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
	osThreadTerminate(osThreadGetId());
	
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
