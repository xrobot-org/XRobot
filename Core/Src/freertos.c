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
Task_List_t task_list;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);

/* USER CODE BEGIN 2 */
__weak void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

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
  
	osThreadDef(cli,			Task_CLI,			osPriorityBelowNormal,	0, 128);
	osThreadDef(comm,			Task_Comm,			osPriorityHigh,			0, 128);
	osThreadDef(ctrl_chassis,	Task_CtrlChassis,	osPriorityAboveNormal,	0, 128);
	osThreadDef(ctrl_gimbal,	Task_CtrlGimbal,	osPriorityAboveNormal,	0, 128);
	osThreadDef(ctrl_shoot,		Task_CtrlShoot,		osPriorityAboveNormal,	0, 128);
	osThreadDef(debug,			Task_Debug,			osPriorityRealtime,		0, 128);
	osThreadDef(info,			Task_Info,			osPriorityBelowNormal,	0, 128);
	osThreadDef(monitor,		Task_Monitor,		osPriorityNormal,		0, 128);
	osThreadDef(pos_esti,		Task_PosEsti,		osPriorityRealtime,		0, 128);
	osThreadDef(referee,		Task_Referee,		osPriorityNormal,		0, 128);
	
	//task_list.cli			= osThreadCreate(osThread(cli),				&task_list);
	//task_list.comm			= osThreadCreate(osThread(comm),			&task_list);
	//task_list.ctrl_chassis	= osThreadCreate(osThread(ctrl_chassis),	&task_list);
	//task_list.ctrl_gimbal	= osThreadCreate(osThread(ctrl_gimbal),		&task_list);
	//task_list.ctrl_shoot	= osThreadCreate(osThread(ctrl_shoot),		&task_list);
	//task_list.debug			= osThreadCreate(osThread(debug),			&task_list);
	task_list.info			= osThreadCreate(osThread(info),			&task_list);
	//task_list.monitor		= osThreadCreate(osThread(monitor),			&task_list);
	task_list.pos_esti		= osThreadCreate(osThread(pos_esti),		&task_list);
	//task_list.referee		= osThreadCreate(osThread(referee),			&task_list);
	
	
	#if defined ROBOT_TYPE_INFANTRY
		
	#elif defined ROBOT_TYPE_HERO
		
	#elif defined ROBOT_TYPE_ENGINEER
		
	#elif defined ROBOT_TYPE_DRONE
		
	#elif defined ROBOT_TYPE_SENTRY

	#else
		
		#error: Must define ROBOT_TYPE_XXXX.
		
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
