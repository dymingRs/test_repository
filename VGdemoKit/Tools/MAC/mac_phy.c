#include <math.h>
#include "radio.h"
#include "sx126x.h"
#include "sx126x-board.h"
#include "lora.h"
#include "mac.h"
#include "malloc.h"

#define USE_MODEM_LORA //LORAģʽ
//#define USE_MODEM_FSK//FSKģʽ

#define RF_FREQUENCY                                433920000 // Hz
#define TX_OUTPUT_POWER                             5        // max=22 dBm

//extern bool IrqFired;
//uint16_t  crc_value;
/*!
* Radio events function pointer
*/
static RadioEvents_t RadioEvents;

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              1       // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       10         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8       // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0        // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

//#define FSK_FDEV                                    5e3      // Hz 
//#define FSK_DATARATE                                2.4e3      // bps
//#define FSK_BANDWIDTH                               20e3     // Hz >> DSB in sx126x
//#define FSK_AFC_BANDWIDTH                           100e3     // Hz
//#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
//#define FSK_FIX_LENGTH_PAYLOAD_ON                   false


#define FSK_FDEV                                    125e3      // Hz 
#define FSK_DATARATE                                250e3      // bps
#define FSK_BANDWIDTH                               300e3     // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH                           100e3     // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
#error "Please define a modem in the compiler options."
#endif


#define RX_TIMEOUT_VALUE                            3000
#define BUFFER_SIZE                                 255 // Define the payload size here

//CAD parameters
#define CAD_SYMBOL_NUM          LORA_CAD_04_SYMBOL
#define CAD_DET_PEAK            23
#define CAD_DET_MIN             10
#define CAD_TIMEOUT_MS          2000
#define NB_TRY                  10


//const uint8_t PingMsg[] = "PING\r\n";
//const uint8_t PongMsg[] = "PONG";
//uint16_t BufferSize = BUFFER_SIZE;
//uint8_t TX_Buffer[BUFFER_SIZE];
//States_t State = LOWPOWER;
//static States_t State = LOWPOWER;
//int16_t RssiValue = 0;
//int16_t SnrValue = 0;

/********************************************************
static function declare
*********************************************************/
static void OnTxDone( void );
static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnTxTimeout( void );
static void OnRxTimeout( void );
static void OnRxError( void );
static void OnCadDone( bool channelActivityDetected);
void mac_phy_task_running( mac_obj_t *obj );

/********************************************************
static varabale declare
*********************************************************/
static uint8_t      RX_Buffer[BUFFER_SIZE];
//static uint8_t      * pMsg;
//static uint16_t     msg_len;
//static States_t     *pState;
//static Mac_t        *pMacObj;
static AppCallBack  mac_callback_func;
//static LpCallBack   app_lp_callback_func = NULL;
static bool         cad_done_flag = false;
static bool         cad_det_flag =false;
static RadioLoRaCadSymbols_t cad_symbols;
static uint8_t cadDetPeak;
static uint8_t cadDetMin;
static double symbol_time;

void SX126xConfigureCad( RadioLoRaCadSymbols_t cadSymbolNum, uint8_t cadDetPeak, uint8_t cadDetMin , uint32_t cadTimeout)
{
    SX126xSetDioIrqParams( 	IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED, IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                            IRQ_RADIO_NONE, IRQ_RADIO_NONE );
    SX126xSetCadParams( cadSymbolNum, cadDetPeak, cadDetMin, LORA_CAD_ONLY, ((cadTimeout * 1000) / 15.625 ));
}

void mac_phy_radio_deint( void )
{ 
  // Radio initialization
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = OnCadDone;
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  
#if defined( USE_MODEM_LORA )
  
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  
  
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  
  
#elif defined( USE_MODEM_FSK )
  
  Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                    FSK_DATARATE, 0,
                    FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, 0, 3000 );
  
  Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                    0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                    0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                    0, 0,false, true );
#else
#error "Please define a frequency band in the compiler options."
#endif
 
}


void mac_phy_radio_ready( mac_obj_t *obj )
{
    SX126xReset( );

    SX126xWakeup( );
    SX126xSetStandby( STDBY_RC );
}

void mac_phy_channel_set( uint32_t freq )
{
       Radio.SetChannel( freq );
}

void mac_phy_power_set( int8_t power )
{
    SX126xSetRfTxPower( power );
}

/*!
* \brief    mac_phy射频的SPI通讯与单载波测试函数
* \remark   
*          
* \param    [in] freq       频点
* \param    [in] power      发射功率   
* \retval   void
*/
void mac_phy_radio_test( uint32_t freq, int8_t power )
{
  uint8_t data[2]={0x12,0x34};
  uint8_t test[2];
  
  // Radio initialization
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = OnCadDone;
  
  Radio.Init( &RadioEvents );
  Radio.WriteBuffer(0x06C0,data,2);
  Radio.ReadBuffer(0x06C0,test,2);
  while( memcmp( data, test, 2) != 0 );
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( freq );
  
  Radio.SetTxConfig( MODEM_LORA, power, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  
  
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  
  RADIO_PA_EN();
  Radio.SetTxContinuousWave(freq,power,0);

  while(1);
}

/*!
* \brief    mac_phy参数初始化
* \remark   

* \param    [in] obj        mac对象指针
* \param    [in] pk         数据包指针
* \param    [in] len        数据包长度
* \param    [in] callback   aplication回调函数
* \param    [in] lpcb       mac phy低功耗回调函数
* \param    [in] s          mac工作状态指针

* \retval   void     
*/
//void mac_phy_int( Mac_t *obj, uint8_t  * pk, uint16_t len, AppCallBack callback, LpCallBack lpcb, States_t *s )
void mac_phy_int( AppCallBack callback )
{
    mac_callback_func=callback; //rf task callback function for mac
}


/*!
* \brief    mac_phy rf参数配置函数
* \remark   

* \param    [in] freq       rf工作频点    
* \param    [in] power      rf 发射功率
* \param    [in] sf         rf扩频因子
* \param    [in] bandwidth  rf带宽
* \param    [in] coderate   rf编码率code rate 
* \param    [in] per        mac LDC低功耗功能cadPreamble period 

* \retval   void     
*/
void mac_phy_radio_config( mac_obj_t *obj, uint32_t freq, int8_t power,uint32_t sf, uint32_t bandwidth, uint8_t coderate, uint32_t per )
{ 
  uint32_t bw;
  uint16_t preambleLen;
  
  Radio.Standby();
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( freq );
  
  if( bandwidth ==0 )//125khz
  {
    switch( sf )
    {
    case 7: cad_symbols=LORA_CAD_02_SYMBOL; cadDetPeak =22; cadDetMin =10; break;
    case 8: cad_symbols=LORA_CAD_02_SYMBOL; cadDetPeak =22; cadDetMin =10; break;
    case 9: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =23; cadDetMin =10; break;
    case 10: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =24; cadDetMin =10; break;
    case 11: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =25; cadDetMin =10; break;
    case 12: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =28; cadDetMin =10; break;
    default:break;
    }
  }
  else if( bandwidth==1 )//250khz
  {
    
  }
  else if( bandwidth==2 )//500khz
  {
    switch( sf )
    {
    case 7: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =21; cadDetMin =10; break;
    case 8: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =22; cadDetMin =10; break;
    case 9: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =22; cadDetMin =10; break;
    case 10: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =23; cadDetMin =10; break;
    case 11: cad_symbols=LORA_CAD_04_SYMBOL; cadDetPeak =25; cadDetMin =10; break;
    case 12: cad_symbols=LORA_CAD_08_SYMBOL; cadDetPeak =29; cadDetMin =10; break;
    default:break;
    }
  }
     
  if( per==0 )
  {
    Radio.SetTxConfig( MODEM_LORA, power, 0, bandwidth,
                    sf, coderate,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
    
    Radio.SetRxConfig( MODEM_LORA, bandwidth, sf,
                    coderate, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  }
  else
  {
     if( bandwidth ==0 )
     {
        bw = 125;
     }
     else if( bandwidth == 1 )
     {
        bw = 250;
     }
     else if( bandwidth ==2 )
     {
       bw = 500;
     }
     
     symbol_time = exp2(sf)/bw;
     preambleLen = (uint16_t)((per+100)/symbol_time);
     Radio.SetTxConfig( MODEM_LORA, power, 0, bandwidth,
                    sf, coderate,
                    preambleLen, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
     
     Radio.SetRxConfig( MODEM_LORA, bandwidth, sf,
                    coderate, 0, preambleLen,
                    preambleLen-2, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
      
     SX126xConfigureCad( cad_symbols, cadDetPeak,cadDetMin, CAD_TIMEOUT_MS); // Configure the CAD   
  }
}



void  mac_phy_cad_check( mac_obj_t *obj )
{
    SX126xConfigureCad( cad_symbols, cadDetPeak,cadDetMin, CAD_TIMEOUT_MS); // Configure the CAD                                                          

    obj->st = START_CAD;
    
    cad_done_flag = false;
    cad_det_flag = false;
    while(1)
    {
      mac_phy_task_running( obj );
      
      if( cad_done_flag )
      {
        cad_done_flag = false;
        
        if(cad_det_flag)
        {
          cad_det_flag = false;
          obj->st =  START_CAD;
        }
        else
        {
          break;
        }
      }
    }
}


void mac_phy_downlink_task( mac_obj_t *obj )
{
  ListNode_t *list;
  
  if( obj->busy ) return;
  
  list = mac_dwlk_queue_get();
  
  if( list_node_find( list, list->pNext) == NULL ) return;
  obj->buf = (uint8_t *)mymalloc( SRAMIN, list->pNext->size);
  if( list_node_data_pop( list, list->pNext,obj->buf, (uint16_t *)&(obj->len) ) )
  {
     mac_states_set( obj,  START_TX );
  }
}


void mac_phy_resend_task( mac_obj_t *obj  )
{
  ListNode_t *list;
  
  list = mac_dwlk_RemitList_get();
  if( list_node_data_pop( list, list->pNext,obj->buf, (uint16_t *)&(obj->len) ) )
  {
    mac_states_set( obj,  START_TX );
  }
}


/*!
* \brief    mac_phy任务轮训
* \remark   

* \param    void

* \retval   void          
*/
void mac_phy_task_running( mac_obj_t *obj )
{ 
  
  obj_map_infor( obj )->adapter->module_irq_polling();
  
#if MULTI_MODULE_MODE_IS_SEVER
  
    mac_phy_downlink_task( mac_downlink_obj_get() );
    //mac_phy_resend_task( mac_downlink_obj_get() );
  
#endif
  
  switch( obj->st)
  {
  case START_TX: 
    
    RADIO_PA_EN();//PA Enable 
    Radio.Standby();
    Radio.Send( obj->buf, obj->len );
    
    obj->st =TX_RUNNING; 
    
    break;
    
  case ACK_TX_PREPARE:
    
     mac_callback_func( obj, NULL,NULL,NULL,NULL,NULL );
     
    break;
    
  case ACK_START_TX:
    
    RADIO_PA_EN();//PA Enable 
    Radio.Standby();
    Radio.Send( obj->buf, obj->len);
    
    obj->st = ACK_TX_RUNNING; 
    
    break;
    
  case ACK_TX_RUNNING: break;
    
  case TX_RUNNING: break;
    
  case TX_DONE: break;
    
  case TX_FAIL:
    
    mac_callback_func( obj, NULL,NULL,NULL,NULL,NULL);
    
    break;
    
  case START_RX:
    
    RADIO_LNA_EN();//LNA Enable
    Radio.Standby();
    Radio.Rx( RX_TIMEOUT_VALUE ); 
    
    obj->st = RX_RUNNING;
    break;
    
  case ACK_START_RX:
    
    RADIO_LNA_EN();//LNA Enable
    Radio.Standby();
    Radio.Rx( RX_TIMEOUT_VALUE ); 
    
    obj->st = ACK_RX_RUNNING;
    
    break;
    
  case ACK_RX_RUNNING: break;
  
  case ACK_RX_DONE:
    
    mac_callback_func( obj, RX_Buffer,NULL,NULL,NULL,NULL );
    
    break;
  
  case LDC_START_RX:
    
    SX126xConfigureCad( cad_symbols, cadDetPeak,cadDetMin, CAD_TIMEOUT_MS); // Configure the CAD   
    RADIO_LNA_EN();//LNA Enable
    Radio.Standby();
    Radio.StartCad( );//StartCad 
    
    obj->st = LDC_RX_RUNNING;
    
    break;
    
  case LDC_RX_RUNNING:break;
  
  case LDC_RX_DONE:
    
    if( cad_det_flag )
    {
      obj->st = START_RX;
    }
    else
    {
      obj->st = LDC_START_RX;
    }
    
    break;
    
  case RX_RUNNING: break;
  
  case START_CAD:
    
    RADIO_LNA_EN();//LNA Enable
    Radio.Standby();
    Radio.StartCad();//StartCad
    
    obj->st = CAD_RUNNING;
    
    break;
    
  case CAD_RUNNING:break;
  
  case STAND_BY:break;
  
  case SLEEP:
    
    RADIO_PA_LNA_OFF();//PA&LNA Disable
    Radio.Standby();//standby
    Radio.Sleep();//sleep
    
    obj->st = LOWPOWER;
    
    break;
    
  case LOWPOWER: break;
  
  default : break;
  }
}


static void OnTxDone( void )
{   
  mac_obj_t *obj = mac_get_cursor();
   
  Radio.Standby();
  if( obj->st == ACK_TX_RUNNING )
  {
   obj->st = ACK_TX_DONE;
  }
  else
  {
   obj->st = TX_DONE;
  }
  
  mac_callback_func( obj, NULL, NULL, NULL, NULL,NULL );//callback function for mac layer
}

static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{ 
  mac_obj_t *obj = mac_get_cursor();
    
  Radio.Standby();
  
  if( size < MAC_FRAME_LEN )
  {
    obj->st =RX_ERROR;
    return;
  }
  
  memcpy( RX_Buffer, payload, size );
 
  obj->st =RX_DONE;
  mac_callback_func( obj,  RX_Buffer, size, rssi, snr, NULL );//callback function for mac layer
}

static void OnCadDone( bool channelActivityDetected)
{ 
  cad_done_flag = false;
  cad_det_flag = false; 
  mac_obj_t *obj = mac_get_cursor();
  
  Radio.Standby( );
  
  if( channelActivityDetected == true )//CadRx = CAD_SUCCESS; 
  { 
    if( obj->st ==LDC_RX_RUNNING )
    {
       obj->st = LDC_RX_DONE;
       
       cad_det_flag =true;
    }
    else
    {
      cad_done_flag = true;
      cad_det_flag =true;
    }
  }
  else
  {
    if( obj->st == LDC_RX_RUNNING ) 
    {
      obj->st = LDC_RX_DONE;
      cad_det_flag = false;
      
      Radio.Sleep();
      if( obj_map_infor( obj )->adapter->appLpCb )
      {
        obj_map_infor( obj )->adapter->appLpCb( obj );
      }
    }
    else
    {
      cad_done_flag =true;
      cad_det_flag = false;
    }
  }
}

static void OnTxTimeout( void )
{
   Radio.Standby();
   mac_get_cursor()->st = START_RX;
}

static void OnRxTimeout( void )
{
  Radio.Standby();
  mac_get_cursor()->st = START_RX;
}

static void OnRxError( void )
{
  Radio.Standby();
  mac_get_cursor()->st = START_RX;
}

