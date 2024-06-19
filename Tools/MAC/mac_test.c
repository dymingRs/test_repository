#include <stdio.h>
#include <stdbool.h>
#include "string.h"
#include "stm32f1xx_hal.h"
#include "mac.h"
#include "MultiTimer.h"
#include "mac_aes.h"
#include "main.h"
#include "radio.h"
#include "mac_port.h"

#include "sx126x.h"
#include "sx126x-board.h"


//static Mac_t   *pAppMacObj;
//static uint8_t  *pBuf;
//static uint8_t  app_buf[100];
static uint16_t pk_len;
static uint8_t  app_raw_buf[20]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
static uint8_t  app_akc_buf[10] = {1,3,5,7,9,2,4,6,8,0};
static uint8_t  app_aes_key[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static MultiTimer  app_send_timer;

#define isMaster     1

static  mac_obj_t  *obj_table[8]={0};

#define MODULE_ONE    obj_table[0]
#define MODULE_TWO    obj_table[1]

void app_send_timer_callback(MultiTimer* timer, void* userData)
{
   mac_obj_t *obj=(mac_obj_t*)userData;
   
   MultiTimerStart(timer,5000, app_send_timer_callback, obj );
   
   if(  Mac.BusyGet( obj )== false )
   {
     //Mac.AsSender( obj, MAC_TYPE_DATA );
     Mac.PkMake( obj, MAC_TYPE_DATA, app_raw_buf, 20, &pk_len);  
     Mac.SSet( obj, START_TX);
   }
}



void mac_ack_start_rx_hook( mac_obj_t *obj )
{
#if MULTI_MODULE_MODE_IS_CLIENT
  

  
#else
//        Mac.AttributeOfReciver( obj, //mac object pointer
//                      NULL, //ack buffer pointer  app_akc_buf
//                      0, //ack msg len
//                      obj->mac.mac_head.app_id, //app_id
//                      obj->mac.mac_head.dev_id, //dev_id
//                      0, //ch_id
//                      true, //app_id filter
//                      true, //aes decrypt enable control
//                      app_aes_key);// aes key pointer 
        
#endif   
        
        Mac.AsReciver( obj );
}



void mac_ack_msg_update_hook( mac_obj_t *obj )
{
    
    
}

void mac_app_callback( mac_obj_t *obj, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr,void *arg )
{
    switch( obj->st )
    {
    case TX_DONE:

         printf("send ok\r\n");

      break;
      
    case ACK_TX_PREPARE:
      
      
      break;
      
    case RX_DONE:
      
      printf( "dev %p: \r\n", &(obj->node) );
      for(uint8_t i =0 ;i<size; i++)
      {
       printf("%02x  ",payload[i]);
      }
      printf("\r\n");

      break;
      
    case ACK_RX_DONE:

      if( payload )
      {
        printf("ack body:\r\n");
        for(uint8_t i =0 ;i<size; i++)
        {
         printf("%#x  ",payload[i]);
        }
        printf("\r\n");
      }
      else
      {
        printf("ack with No Msg\r\n");
      }

      printf("ack success\r\n");
  
      break;    
      
    case RX_ERROR:
      
         break;
         
    case TX_FAIL:
      
      
      printf("ack mode tx fail\r\n");
      
        break;
        
       default:break;
    }
}

void mac_app_lp_callback( mac_obj_t *obj )
{
 
 HAL_Delay(500);
 
}


static  mac_adapter_t adapter_table[]={
 
{ module_I_nss_ctr,  module_I_rst_ctr, module_I_busy_check,  module_I_irq_polling,   mac_app_callback, mac_app_lp_callback, {1,"md_1"} },

#if MULTI_MODULE_MODE_IS_SEVER

{ module_II_nss_ctr, module_II_rst_ctr, module_II_busy_check, module_II_irq_polling, mac_app_callback, mac_app_lp_callback, {2,"md_2"} },
 

#endif
};


void mac_app_test()
{
  Mac.Int();//mac int
  
  for( uint8_t i=0;i< sizeof(adapter_table)/sizeof(adapter_table[0]);i++ )
  {
    obj_table[i] = Mac.ObjCreat( &(adapter_table[i]) );//creat a object
    Mac.Radio_Adapter.RadioReady( obj_table[i] );//waitting radio to be ready
  }
  
  for( uint8_t i=0;i< sizeof(adapter_table)/sizeof(adapter_table[0]);i++ )
  {
    Mac.Radio_Adapter.RadioDeInt( obj_table[i] );//radio default config
  }
   
 //Mac.RfTest( MODULE_ONE, 433000000, 22 );
 
#if  MULTI_MODULE_MODE_IS_CLIENT
  
 {
  Mac.Radio_Adapter.RadioUpDownChTbSet( MODULE_ONE, 435920000, 3, 433920000, 433420000, 436320000 );//updownlink channel tabel set
  Mac.Radio_Adapter.RadioUpDownChIdTbSet( MODULE_ONE, 1, 3, 0, 23, 45 );//updownlink channel id tabel set
  
  Mac.Radio_Adapter.RadioConfig( MODULE_ONE,//mac object pointer
                                Mac.Radio_Adapter.RadioUpLinkChGet( MODULE_ONE, 0),//frequency
                                0, //power
                                9, //sf
                                0, //bw[0: 125 kHz,//  1: 250 kHz,//  2: 500 kHz,//  3: Reserved]
                                1, //cr
                                0 );// cadPreamble period
  
  Mac.AttributeOfSender( MODULE_ONE, //mac object pointer
                        0xA0A0, //app_id
                        0x56783412, //dev_id
                        Mac.Radio_Adapter.RadioUpLinkChIdGet( MODULE_ONE, 0 ),//ch_id
                        0x11223344, //des_id
                        true, //ack 
                        1000, //timeout
                        2, //repeat
                      //  MAC_TYPE_DATA,//data type
                        true, //aes encrypt enable control
                        app_aes_key); // aes key pointer 
  
  Mac.AttributeOfReciver( MODULE_ONE, //mac object pointer
                         NULL, //ack buffer pointer  app_akc_buf
                         0, //ack msg len
                         0xA0A0, //app_id
                         0x56783412, //dev_id
                         Mac.Radio_Adapter.RadioDownLinkChIdGet( MODULE_ONE ), //ch_id
                         true, //app_id filter
                         true, //aes decrypt enable control
                         app_aes_key);// aes key pointer 
  
  Mac.PkMake( MODULE_ONE, MAC_TYPE_DATA,//mac object pointer
                    app_raw_buf, //user data pointer
                    20,  //user data len 
                    &pk_len ); //mac pk len   
  
  //memcpy( app_buf, pBuf, pk_len);

  Mac.SSet( MODULE_ONE, START_TX );
  
  MultiTimerStart( &app_send_timer, 5000, app_send_timer_callback,MODULE_ONE );

 }
  
#elif  MULTI_MODULE_MODE_IS_SEVER
  
 {
   Mac.Radio_Adapter.RadioConfig( MODULE_ONE,//mac object pointer
                                 433920000,//frequency
                                 0, //power
                                 9, //sf
                                 0, //bw[0: 125 kHz,//  1: 250 kHz,//  2: 500 kHz,//  3: Reserved]
                                 1, //cr
                                 0 );// cadPreamble period
  
  Mac.Radio_Adapter.RadioConfig( MODULE_TWO,//mac object pointer 
                                435920000,//frequency
                                0, //power
                                9, //sf
                                0, //bw[0: 125 kHz,//  1: 250 kHz,//  2: 500 kHz,//  3: Reserved]
                                1, //cr
                                0 );// cadPreamble period 
  
  
  Mac.AttributeOfReciver( MODULE_ONE, //mac object pointer
                NULL, //ack buffer pointer  app_akc_buf
                10, //ack msg len
                0xA0A0, //app_id
                0x11223344, //dev_id
                0, //ch_id
                true, //app_id filter
                true, //aes decrypt enable control
                app_aes_key);// aes key pointer 
  
  Mac.AttributeOfSender( MODULE_TWO, //mac object pointer
               0xA0A0, //app_id
               0x50703010, //dev_id
               1,  //ch_id
               0x51713111, //des_id
               false, //ack 
               1000, //timeout
               2, //repeat
            //   MAC_TYPE_DATA,//data type
               true, //aes encrypt enable control
               app_aes_key); // aes key pointer 
  
  
  Mac.SSet( MODULE_ONE, START_RX ); 
  
  //Mac.AsSender( MODULE_TWO, MAC_TYPE_DATA );
  mac_downlink_obj_set( MODULE_TWO );
  Mac.PkMake( MODULE_TWO, MAC_TYPE_DATA,//mac object pointer
             app_raw_buf, //user data pointer
             20,  //user data len 
             &pk_len ); //mac pk len   
  
  Mac.SSet( MODULE_TWO, STAND_BY ); 
  
 }
 
#else
  
 #if isMaster
    
 {
      Mac.Radio_Adapter.RadioConfig( MODULE_ONE,//mac object pointer
                  433920000,//frequency
                  0, //power
                  9, //sf
                  0, //bw[0: 125 kHz,//  1: 250 kHz,//  2: 500 kHz,//  3: Reserved]
                  1, //cr
                  0 );// cadPreamble period
  
     Mac.AttributeOfSender( MODULE_ONE, //mac object pointer
               0xA0A0, //app_id
               0x56783412, //dev_id
               0,//ch_id
               0x11223344, //des_id
               true, //ack 
               1000, //timeout
               2, //repeat
             //  MAC_TYPE_DATA,//data type
               true, //aes encrypt enable control
               app_aes_key); // aes key pointer 
  
   // Mac.AsSender( MODULE_ONE, MAC_TYPE_DATA );
    Mac.PkMake( MODULE_ONE, MAC_TYPE_DATA, //mac object pointer
                    app_raw_buf, //user data pointer
                    20,  //user data len 
                    &pk_len ); //mac pk len   
  
  //memcpy( app_buf, pBuf, pk_len);
  
  Mac.SSet( MODULE_ONE, START_TX );
  
//  MultiTimerStart( &app_send_timer, 1000, app_send_timer_callback,MODULE_ONE );
 }
 
 #else
   
 { 
    Mac.Radio_Adapter.RadioConfig( MODULE_ONE,//mac object pointer
                  433920000,//frequency
                  0, //power
                  9, //sf
                  0, //bw[0: 125 kHz,//  1: 250 kHz,//  2: 500 kHz,//  3: Reserved]
                  1, //cr
                  0 );// cadPreamble period
  
    Mac.AttributeOfReciver( MODULE_ONE, //mac object pointer
                NULL, //ack buffer pointer  app_akc_buf
                10, //ack msg len
                0xA0A0, //app_id
                0x11223344, //dev_id
                0, //ch_id
                true, //app_id filter
                true, //aes decrypt enable control
                app_aes_key);// aes key pointer 
    
    Mac.AsReciver( MODULE_ONE );
    Mac.SSet( MODULE_ONE, START_RX ); 
 }      
  
 #endif
  
  
#endif
  
  while(1)
  {
    Mac.Running();//mac task running
    
    MultiTimerYield();//multi timer running
  }
}