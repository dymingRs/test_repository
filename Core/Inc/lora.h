#ifndef _LORA_H_
#define _LORA_H_

#include "stm32f1xx_hal.h"
#include "gpio.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>  

#define RADIO_LNA_EN()      HAL_GPIO_WritePin(Radio_LNA_EN_GPIO_Port,Radio_LNA_EN_Pin,GPIO_PIN_SET);\
                            HAL_GPIO_WritePin(Radio_PA_EN_GPIO_Port,Radio_PA_EN_Pin,GPIO_PIN_RESET) 
                       
#define RADIO_PA_EN()       HAL_GPIO_WritePin(Radio_PA_EN_GPIO_Port,Radio_PA_EN_Pin,GPIO_PIN_SET);\
                            HAL_GPIO_WritePin(Radio_LNA_EN_GPIO_Port,Radio_LNA_EN_Pin,GPIO_PIN_RESET) 
                          
#define RADIO_PA_LNA_OFF()  HAL_GPIO_WritePin(Radio_PA_EN_GPIO_Port,Radio_PA_EN_Pin,GPIO_PIN_RESET);\
                            HAL_GPIO_WritePin(Radio_LNA_EN_GPIO_Port,Radio_LNA_EN_Pin,GPIO_PIN_RESET) 
                         
#define BOARD_LED0_ON()   HAL_GPIO_WritePin(Board_LED0_GPIO_Port,Board_LED0_Pin,GPIO_PIN_RESET)
#define BOARD_LED0_OFF()  HAL_GPIO_WritePin(Board_LED0_GPIO_Port,Board_LED0_Pin,GPIO_PIN_SET) 
    
    
void board_rf_config(void);
void board_rf_task(void);



#endif 
