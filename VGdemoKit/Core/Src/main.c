/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <LowLevelIOInterface.h>
#include "lora.h"
#include "malloc.h"
#include "list.h"
#include <flashdb.h>
#include <fdb_def.h>
#include "MultiTimer.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define FLASH_USER_START_ADDR   ADDR_FLASH_PAGE_100   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_127 + FLASH_PAGE_SIZE   /* End @ of user Flash area */

uint32_t Address = 0, PAGEError = 0;
__IO uint32_t data32 = 0 , MemoryProgramStatus = 0;

/*Variable used for Erase procedure*/
static FLASH_EraseInitTypeDef EraseInitStruct;

#define DATA_32                 ((uint32_t)0x12345678)



typedef struct 
{
    uint8_t a;//1
    uint16_t b;//2
    uint32_t c;//4
}__attribute__((aligned(8))) FlashTest_t ; 
 
 FlashTest_t data_test=
 {
  .a=1,
  .b=0x1234,
  .c=3,
 
 };
 
 FlashTest_t data_test2;
 
 uint8_t buf[50];
 
 uint8_t num=0;
 
 void flash_test()
 {
     /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
  
    /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
  EraseInitStruct.NbPages     = (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR) / FLASH_PAGE_SIZE;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    
    
  }
  
    /* Program the user Flash area word by word
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/
  
  
   uint8_t * pTemp=(uint8_t *)(&data_test);
  
  Address = FLASH_USER_START_ADDR;
  for(uint8_t i=0;i<sizeof(FlashTest_t);i++)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *pTemp);
    pTemp++;
    Address+=4;
  }

    Address = FLASH_USER_START_ADDR;
   pTemp=(uint8_t *)(&data_test2);
  for(uint8_t i=0;i<sizeof(FlashTest_t);i++)
  {
    pTemp[i]=*(__IO uint8_t *)Address;
   Address+=4;
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
  
 }
 
 
static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
static uint8_t  my_age=18;
static uint8_t  rf_ch=0;
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
        {"username", "armink", 0}, /* string KV */
        {"password", "123456", 0}, /* string KV */
        {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
        {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
        
        {"myname", "dyming", 0}, /* string KV */
        {"mypassword", "abcdef", 0}, /* string KV */
        {"myage", &my_age, sizeof(my_age)}, /* int type KV */
        
        {"Nd1_SF","SF10"},/*string KV*/
        {"Nd1_CH",&rf_ch,sizeof(rf_ch)},
        {"Nd1_BW","250KHz",0}/*int type KV*/
        
        
};




/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

/* TSDB object */
struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;


void kvdb_basic_sample(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
    int boot_count = 0;
    uint8_t ages;


    printf("==================== kvdb_basic_sample ====================\r\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            printf("get the 'boot_count' value is %d\r\n", boot_count);
        } else {
            printf("get the 'boot_count' failed\r\n");
        }
    }
    
    { /* GET the KV value */
      /* get the "ages" KV value */
      fdb_kv_get_blob(kvdb, "myage", fdb_blob_make(&blob, &ages, sizeof(ages)));
      /* the blob.saved.len is more than 0 when get the value successful */
      if (blob.saved.len > 0) {
        printf("get the 'ages' value is %d\r\n", ages);
      } else {
        printf("get the 'ages' failed\r\n");
      }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        boot_count ++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        printf("set the 'boot_count' value to %d\r\n", boot_count);
    }
        
        
   { /* CREATE new Key-Value */
        char temp_data[10] = "36C";

        /* It will create new KV node when "temp" KV not in database. */
        fdb_kv_set(kvdb, "temp", temp_data);
        printf("create the 'temp' string KV, value is: %s\r\n", temp_data);
    }

    { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(kvdb, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            printf("get the 'temp' value is: %s\r\n", temp_data);
        }
    }

    { /* CHANGE the KV value */
        char temp_data[10] = "38C";

        /* change the "temp" KV's value to "38.1" */
        fdb_kv_set(kvdb, "temp", temp_data);
        printf("set 'temp' value to %s\r\n", temp_data);
    }
    
     { /* GET the KV value */
        char *return_value, temp_data[10] = { 0 };

        /* Get the "temp" KV value.
         * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
         */
        return_value = fdb_kv_get(kvdb, "temp");
        /* the return value is NULL when get the value failed */
        if (return_value != NULL) {
            strncpy(temp_data, return_value, sizeof(temp_data));
            printf("get the 'temp' value is: %s\r\n", temp_data);
      }
    }
    
    
    { /* GET the KV value */
      char *return_value, temp_data[10] = { 0 };
      
      /* Get the "myname" KV value.
      * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
      */
      return_value = fdb_kv_get(kvdb, "myname");
      /* the return value is NULL when get the value failed */
      if (return_value != NULL) {
        strncpy(temp_data, return_value, sizeof(temp_data));
        printf("get the 'myname' value is: %s\r\n", temp_data);
      }
    }
    
        
    { /* GET the KV value */
      char *return_value, temp_data[10] = { 0 };
      
      /* Get the "Nd1_BW" KV value.
      * NOTE: The return value saved in fdb_kv_get's buffer. Please copy away as soon as possible.
      */
      return_value = fdb_kv_get(kvdb, "Nd1_BW");
      /* the return value is NULL when get the value failed */
      if (return_value != NULL) {
        strncpy(temp_data, return_value, sizeof(temp_data));
        printf("get the 'Nd1_BW' value is: %s\r\n", temp_data);
      }
    }
     
    printf("===========================================================\r\n");
}

void flashDB_test()
{
     fdb_err_t result;

    /* KVDB Sample */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        
        /* set the lock and unlock function if you want */
     //   fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
      //  fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
        
        /* Key-Value database initialization
        *
        *       &kvdb: database object
        *       "env": database name
        * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
        *              Please change to YOUR partition name.
        * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
        *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

        if (result != FDB_NO_ERR) {
            return ;
        }
     
       /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
}

extern void mac_app_test();
extern void at_test();
void blink_test(void);

uint64_t PlatformTicksGetFunc(void)
{
  return HAL_GetTick();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  
  MultiTimerInstall(PlatformTicksGetFunc);
  my_mem_init(SRAMIN); 
  
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  printf("\n\r UART Printf Example: retarget the C library printf function to the UART\n\r");
  
  //board_rf_config();
  
  //num=sizeof(FlashTest_t);
 /* uint8_t * pTemp=(uint8_t *)(&data_test);
  for(uint8_t i=0;i<num;i++)
  {
   buf[i]=*pTemp;
   pTemp++;
  }
  */
  
  //list_test();
  //flash_test();
  //flashDB_test();
   mac_app_test();
  //at_test();
  //blink_test();
   
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if 0
  board_rf_task();
  
#endif
  } 
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* support printf function, usemicrolib is unnecessary */
#if (__ARMCC_VERSION > 6000000)
  __asm (".global __use_no_semihosting\n\t");
  void _sys_exit(int x)
  {
    x = x;
  }
  /* __use_no_semihosting was requested, but _ttywrch was */
  void _ttywrch(int ch)
  {
    ch = ch;
  }
  FILE __stdout;
#else
 #ifdef __CC_ARM
  #pragma import(__use_no_semihosting)
  struct __FILE
  {
    int handle;
  };
  FILE __stdout;
  void _sys_exit(int x)
  {
    x = x;
  }
  /* __use_no_semihosting was requested, but _ttywrch was */
  void _ttywrch(int ch)
  {
    ch = ch;
  }
 #endif
#endif

#if defined (__GNUC__) && !defined (__clang__)
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
 // #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define  PUTCHAR_PROTOTYPE int MyLowLevelPutchar(int ch)
#endif

/**
  * @brief  retargets the c library printf function to the usart.
  * @param  none
  * @retval none
  */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart1,(uint8_t *)&ch, 1, 0xFFFF);
  
  return ch;
}

#if defined (__GNUC__) && !defined (__clang__)
int _write(int fd, char *pbuffer, int size)
{
  for(int i = 0; i < size; i ++)
  {
    __io_putchar(*pbuffer++);
  }

  return size;
}
#endif


#pragma module_name = "?__write"
/*
int MyLowLevelPutchar(int x)
{
  comSendChar(COM1, x);
  
  return x;

}*/

/*
* If the __write implementation uses internal buffering, uncomment
* the following line to ensure that we are called with "buffer" as 0
* (i.e. flush) when the application terminates.
*/

size_t __write(int handle, const unsigned char * buffer, size_t size)
{
  /* Remove the #if #endif pair to enable the implementation */
#if 1

  size_t nChars = 0;

  if (buffer == 0)
  {
    /*
     * This means that we should flush internal buffers.  Since we
     * don't we just return.  (Remember, "handle" == -1 means that all
     * handles should be flushed.)
     */
    return 0;
  }

  /* This template only writes to "standard out" and "standard err",
   * for all other file handles it returns failure. */
  if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR)
  {
    return _LLIO_ERROR;
  }

  for (/* Empty */; size != 0; --size)
  {
    if (MyLowLevelPutchar(*buffer++) < 0)
    {
      return _LLIO_ERROR;
    }

    ++nChars;
  }

  return nChars;

#else

  /* Always return error code when implementation is disabled. */
  return _LLIO_ERROR;

#endif

}





/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
