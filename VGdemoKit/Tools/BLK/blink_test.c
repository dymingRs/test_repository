#include "stm32f1xx_hal.h"
#include  "malloc.h"
#include "blink.h"
#include "main.h"
#include "linux_list.h"
#include <stdio.h>

/*led工作模式 ---------------------------------------------------------------*/
#define LED_MODE_OFF     0                            /*常灭*/
#define LED_MODE_ON      1                            /*常亮*/
#define LED_MODE_FAST    2                            /*快闪*/
#define LED_MODE_SLOW    3                            /*慢闪*/

/*led类型 --------------------------------------------------------------------*/
typedef enum {
    LED_TYPE_RED = 0,
    LED_TYPE_GREEN,
    LED_TYPE_BLUE,
    LED_TYPE_MAX
}led_type;


static blink_dev_t led[LED_TYPE_MAX];               /*定义led设备 ------------*/

/* 
 * @brief       红色LED控制
 * @return      none
 */ 
static void red_led_ctrl(bool level)
{ 
  if(level)
    HAL_GPIO_WritePin( Board_LED1_GPIO_Port, Board_LED1_Pin, GPIO_PIN_SET );
  else
    HAL_GPIO_WritePin( Board_LED1_GPIO_Port, Board_LED1_Pin, GPIO_PIN_RESET );
}

/* 
 * @brief       蓝色LED控制
 * @return      none
 */ 
static void blue_led_ctrl(bool level)
{
  if(level)
  HAL_GPIO_WritePin( Board_LED0_GPIO_Port, Board_LED0_Pin, GPIO_PIN_SET );
  else
   HAL_GPIO_WritePin( Board_LED0_GPIO_Port, Board_LED0_Pin, GPIO_PIN_RESET );
}



/* 
 * @brief       led控制
 * @param[in]   type    - led类型(LED_TYPE_XXX)
 * @param[in]   mode    - 工作模式(LED_MODE_XXX)
 * @param[in]   reapeat - 闪烁次数,0表示无限循环
 * @return      none
 */ 
void led_ctrl(led_type type, int mode, int reapeat)
{
    int ontime = 0, offtime = 0;
    
    switch (mode) {                     /*根据工作模式得到led开关周期 ---------*/
    case LED_MODE_OFF:
        ontime = offtime = 0;
        break;
        
    case LED_MODE_ON:
        ontime  = 1;
        offtime = 0;
        break;
        
    case LED_MODE_FAST:
        ontime  = 100;
        offtime = 100;
        break;
        
    case LED_MODE_SLOW:
        ontime  = 500;
        offtime = 500;
        break;        
    }
    
    blink_dev_ctrl(&led[type], ontime, offtime, reapeat, HAL_GetTick );
}

typedef struct{
  
  struct list_head node;
  void * p;
  uint8_t size;
  
}list_node_t;

list_node_t         *temp_node, node;
struct list_head    my_list;


#include <stdarg.h>
#include <stdlib.h>
#define SBUF_SIZE 128
char sbuf[SBUF_SIZE] = {75,0x49,0x55};
 
void MyPrintF( const char * format, ... )
{
    int size;
    
	va_list args;
	va_start (args, format);
	size = vsnprintf (sbuf,SBUF_SIZE,format, args);
	va_end (args);
  
	printf("%s",sbuf);		
}

void blink_test(void)
{
  int it =123;
  char dBuf[10];
  it = atoi("12");

 /// printf("%c",sbuf[0]);
  sprintf( dBuf,"%c",sbuf[0]);
  
	MyPrintF("my name is %s,my age is %d\n","bob",18);
    
    memset( &my_list,0,sizeof(my_list));
    INIT_LIST_HEAD( &my_list );
    for(uint8_t i=0; i< 3; i++ )
    {
     temp_node = mymalloc( SRAMIN, sizeof( list_node_t ) );
     list_add_tail( &(temp_node->node), &my_list );
    }
    
    (( list_node_t *)(my_list.next))->size=10;
    memcpy(&node,my_list.next,sizeof( list_node_t ));
    
    struct list_head  *pos;
    list_node_t  *ptr;
        
//    list_del( (my_list.next->next) );
    
    list_for_each( pos, &my_list )
    {
      ptr = list_entry( pos, list_node_t, node  );
      printf( "node obj:%p\r\n",ptr);
    }
   
    char str[80];
    
    float pi =3.14;
    sprintf( str,"%f",3.14);
    printf("%f",pi);
    
    blink_dev_create(&led[LED_TYPE_RED], red_led_ctrl);
    blink_dev_create(&led[LED_TYPE_BLUE], blue_led_ctrl);

    led_ctrl( LED_TYPE_RED, LED_MODE_FAST,10);
    led_ctrl( LED_TYPE_BLUE, LED_MODE_SLOW,20);
    
    while(1)
    {
        blink_dev_process( HAL_GetTick );
    
    }
}