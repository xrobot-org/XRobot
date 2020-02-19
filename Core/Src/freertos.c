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

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

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
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  
	osThreadDef(cli,			Task_CLI,			osPriorityLow,			0, 256);
	osThreadDef(command,		Task_Command,		osPriorityHigh,			0, 128);
	osThreadDef(ctrl_chassis,	Task_CtrlChassis,	osPriorityAboveNormal,	0, 256);
	osThreadDef(ctrl_gimbal,	Task_CtrlGimbal,	osPriorityAboveNormal,	0, 256);
	osThreadDef(ctrl_shoot,		Task_CtrlShoot,		osPriorityAboveNormal,	0, 128);
	osThreadDef(info,			Task_Info,			osPriorityBelowNormal,	0, 128);
	osThreadDef(monitor,		Task_Monitor,		osPriorityNormal,		0, 128);
	osThreadDef(pos_esti,		Task_PosEsti,		osPriorityRealtime,		0, 256);
	osThreadDef(referee,		Task_Referee,		osPriorityNormal,		0, 128);
	
	task_param.thread.cli			= osThreadCreate(osThread(cli),				&task_param);
	task_param.thread.command		= osThreadCreate(osThread(command),			&task_param);
	task_param.thread.ctrl_chassis	= osThreadCreate(osThread(ctrl_chassis),	&task_param);
	task_param.thread.ctrl_gimbal	= osThreadCreate(osThread(ctrl_gimbal),		&task_param);
	task_param.thread.ctrl_shoot	= osThreadCreate(osThread(ctrl_shoot),		&task_param);
	task_param.thread.info			= osThreadCreate(osThread(info),			&task_param);
	task_param.thread.monitor		= osThreadCreate(osThread(monitor),			&task_param);
	task_param.thread.pos_esti		= osThreadCreate(osThread(pos_esti),		&task_param);
	//task_param.thread.referee		= osThreadCreate(osThread(referee),			&task_param);
	
	
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
void StartDefaultTask(void const * argument)
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
