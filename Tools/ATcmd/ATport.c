#include "malloc.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Custom malloc for AT component.
 */
void *at_malloc(unsigned int nbytes)
{
  return mymalloc(SRAMIN,nbytes);
}

/**
 * @brief Custom free for AT component.
 */
void  at_free(void *ptr)
{
  myfree(SRAMIN,(uint8_t*)ptr);
}

/**
 * @brief Gets the total number of milliseconds in the system.
 */
unsigned int at_get_ms(void)
{
  return HAL_GetTick();
  
}