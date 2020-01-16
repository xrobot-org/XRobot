#pragma once

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"

/* Exported constants --------------------------------------------------------*/
/* Exported defines ----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

osMessageQueueId_t motor_feedback_queue;
osMessageQueueId_t uwb_feedback_queue;
osMessageQueueId_t supercap_feedback_queue;


/* Exported functions prototypes ---------------------------------------------*/

void Task_CAN(const void* argument);
