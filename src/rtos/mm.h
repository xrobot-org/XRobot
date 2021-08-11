#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
void *BSP_Malloc(size_t size);
void BSP_Free(void *pv);
