#include "radio.h"
#include "sx126x.h"
#include "sx126x-board.h"
#include "crc.h"
#include "lora.h"



/**************************************************************************************************************************************
Demo 程序流程  EnableMaster=true  为主机端，主机端发送一个"PING"数据后切换到接收，等待从机返回的应答"PONG"数据LED闪烁

EnableMaster=false 为从机端，从机端接收到主机端发过来的"PING"数据后LED闪烁并发送一个"PONG"数据作为应答
***************************************************************************************************************************************/

#define USE_MODEM_LORA //LORA模式
//#define USE_MODEM_FSK//FSK模式
bool EnableMaster=true;//主从选择

#define REGION_CN779

#if defined( REGION_AS923 )

#define RF_FREQUENCY                                923000000 // Hz

#elif defined( REGION_AU915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_CN779 )

#define RF_FREQUENCY                                490000000 // Hz

#elif defined( REGION_EU868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( REGION_KR920 )

#define RF_FREQUENCY                                920000000 // Hz

#elif defined( REGION_IN865 )

#define RF_FREQUENCY                                865000000 // Hz

#elif defined( REGION_US915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_US915_HYBRID )

#define RF_FREQUENCY                                915000000 // Hz

#else

#error "Please define a frequency band in the compiler options."

#endif

#define TX_OUTPUT_POWER                             0        // 22 dBm

extern bool IrqFired;


uint16_t  crc_value;
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


typedef enum
{
  LOWPOWER,
  RX,
  RX_TIMEOUT,
  RX_ERROR,
  START_TX,
  TX,
  TX_TIMEOUT,
  START_CAD,
  START_RX,
}States_t;

typedef enum
{
  CAD_FAIL,
  CAD_SUCCESS,
  PENDING,
}CadRx_t;

CadRx_t CadRx = CAD_FAIL;
bool PacketReceived = false;

#define RX_TIMEOUT_VALUE                            3000
#define BUFFER_SIZE                                 255 // Define the payload size here

const uint8_t PingMsg[] = "PING\r\n";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t TX_Buffer[BUFFER_SIZE]="0123456789QABRJEIIEJFEIHGETJHGJKSKGHEJFEWIKFJFEDSHFEEGFASGTEWTGEWG\
EGEGEWWG45567RFTGYHVFDCFGVHGBSDGFTGEWRTGVSDGSDTGSEGFSGSDGSDGRSDGRGSRDGVD678990076543212345YHGHGFTT6\
7ervyyjhjjkGDGDDF0909876655321122346790ijyuhbvfr445567980907hghgg990875fu5dvy7ybuhybntre";

uint8_t RX_Buffer[BUFFER_SIZE];


States_t State = LOWPOWER;

int16_t RssiValue = 0;
int16_t SnrValue = 0;
void OnTxDone( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( void );
void OnRxTimeout( void );
void OnRxError( void );



//#define SPI_TEST
//#define Radio_TS
void board_rf_config(void)
{
  
#ifdef SPI_TEST
  
  uint8_t data[2]={0x12,0x34};
  static uint8_t test[2];
  
#endif
  
  // Radio initialization
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  
#ifdef  SPI_TEST
  
  Radio.WriteBuffer(0x06C0,data,2);
  Radio.ReadBuffer(0x06C0,test,2);
  
#endif
  
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
  
#ifdef Radio_TS
  
  RADIO_PA_EN();
  Radio.SetTxContinuousWave(433100000,22,0);
  
  while(1);
  
#endif
  
  if(EnableMaster)
  {
//    TX_Buffer[0] = 'P';
//    TX_Buffer[1] = 'I';
//    TX_Buffer[2] = 'N';
//    TX_Buffer[3] = 'G'; 
//    
//    crc_value=RadioComputeCRC(TX_Buffer,4,CRC_TYPE_IBM);//计算得出要发送数据包CRC值
//    TX_Buffer[4]=crc_value>>8;
//    TX_Buffer[5]=crc_value;
//    for(uint16_t i=0;i<255;i++)
//    {
//      TX_Buffer[i]=i+33;
//      
//    }
    //TX_Buffer="0123456789ABCDEFJLJIEUFHIODNNFEFEKFKDNFVDDSV";
    memcpy(TX_Buffer,PingMsg,6);
    Radio.Send( TX_Buffer, 6);
  }
  else
  {
    RADIO_LNA_EN();
    Radio.Rx( RX_TIMEOUT_VALUE ); 
    
  }
}


#if 1
void board_rf_task(void)
{
  
  Radio.IrqProcess(); // Process Radio IRQ
  
  if(EnableMaster)
  {
    switch(State)
    {
    case START_TX:
      
      HAL_Delay(1000);

      BOARD_LED0_ON();
      Radio.Send( TX_Buffer, 6);
      
      State=START_TX;
      
      break;
      
    case LOWPOWER:    break;
    
    default : break;
    
    }
  }
  else
  {
    switch(State)
    {
    case RX_TIMEOUT:
       
       printf("%s\r\n","OnRxTimeout!");
       
       State = START_RX;
      
      break;
      
      
    case RX_ERROR:
      
      printf( "RX Error\r\n");
      
      State = START_RX;
      
      break;
      
      
    case RX:
      
      BOARD_LED0_ON();
      HAL_Delay(20);
      BOARD_LED0_OFF();
      
      printf("%s %ddBm\r\n",RX_Buffer,RssiValue);
      memset(RX_Buffer,0,sizeof(RX_Buffer));
       
       State = START_RX;
       
      break;
      
    case START_RX:
      
      
       Radio.Rx( RX_TIMEOUT_VALUE ); //进入接收
      
      State = LOWPOWER;
      
      break;
      
    case LOWPOWER: break;
        
    default : break;
    
    }
    
  }
  
}


void OnTxDone( void )
{   
  Radio.Standby();
  //Radio.Rx( RX_TIMEOUT_VALUE ); //进入接收
  State = START_TX;
  BOARD_LED0_OFF();
  
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{  
  Radio.Standby();
  
  BufferSize = size;
  memcpy( RX_Buffer, payload, BufferSize );
  RssiValue = rssi;
  SnrValue = snr;
  
  State = RX;
}


void OnTxTimeout( void )
{
  
}

void OnRxTimeout( void )
{
  Radio.Standby();
  
  State = RX_TIMEOUT;
}

void OnRxError( void )
{
    Radio.Standby( );
    
    State = RX_ERROR;
  
}
#endif



