#include "mac.h"
#include "stm32f1xx_hal.h"
#include "radio.h"
#include "sx126x.h"
#include "sx126x-board.h"
#include "gpio.h"
#include "spi.h"
#include "mac_port.h"

void RadioOnDioIrq( void );



void module_I_nss_ctr( mac_port_pin_state_t state )
{
  if( state == SET_LOW ) 
  {
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port,Radio_NSS_Pin,GPIO_PIN_RESET);
  }
  else if( state == SET_HIG )
  {
    HAL_GPIO_WritePin(Radio_NSS_GPIO_Port,Radio_NSS_Pin,GPIO_PIN_SET);
  }
}

void module_I_rst_ctr( mac_port_pin_state_t state )
{
  if( state == SET_LOW )
  {
    HAL_GPIO_WritePin(Radio_Reset_GPIO_Port,Radio_Reset_Pin,GPIO_PIN_RESET);
  }
  else if( state == SET_HIG )
  {
    HAL_GPIO_WritePin(Radio_Reset_GPIO_Port,Radio_Reset_Pin,GPIO_PIN_SET);
  }
}

bool module_I_busy_check( void )
{
  return (HAL_GPIO_ReadPin(Radio_Busy_GPIO_Port,Radio_Busy_Pin)==GPIO_PIN_SET) ? true : false;  
}

void module_I_irq_polling( void )
{
    if(HAL_GPIO_ReadPin(Radio_DIO1_GPIO_Port,Radio_DIO1_Pin)==GPIO_PIN_SET)
    {
     RadioOnDioIrq();
     Radio.IrqProcess(); // Process Radio IRQ
    }
}

void module_II_nss_ctr( mac_port_pin_state_t state )
{
  if( state == SET_LOW )
  {
    HAL_GPIO_WritePin(Radio_NSS_SUB_GPIO_Port,Radio_NSS_SUB_Pin,GPIO_PIN_RESET);
  }
  else if( state == SET_HIG )
  {
    HAL_GPIO_WritePin(Radio_NSS_SUB_GPIO_Port,Radio_NSS_SUB_Pin,GPIO_PIN_SET);
  }
}

void module_II_rst_ctr( mac_port_pin_state_t state )
{
  if( state == SET_LOW )
  {
    HAL_GPIO_WritePin(Radio_Reset_SUB_GPIO_Port,Radio_Reset_SUB_Pin,GPIO_PIN_RESET);
  }
  else if( state == SET_HIG )
  {
    HAL_GPIO_WritePin( Radio_Reset_SUB_GPIO_Port,Radio_Reset_SUB_Pin, GPIO_PIN_SET );
  }
}

bool module_II_busy_check( void )
{
  return (HAL_GPIO_ReadPin( Radio_Busy_SUB_GPIO_Port,Radio_Busy_SUB_Pin )==GPIO_PIN_SET) ? true : false;
}

void module_II_irq_polling()
{
    if( HAL_GPIO_ReadPin(Radio_DIO1_SUB_GPIO_Port,Radio_DIO1_SUB_Pin)==GPIO_PIN_SET )
    {
     RadioOnDioIrq();
     Radio.IrqProcess(); // Process Radio IRQ
    }
}


void RADIO_NSS_L( void )
{
  if( mac_get_cursor() == NULL) return;
   obj_map_infor(mac_get_cursor())->adapter->module_nss_ctr( SET_LOW );
}

void RADIO_NSS_H( void )
{
    if( mac_get_cursor() == NULL) return;
     obj_map_infor(mac_get_cursor())->adapter->module_nss_ctr( SET_HIG );
}

void RADIO_nRESET_L() 
{
    if( mac_get_cursor() == NULL) return;
   obj_map_infor(mac_get_cursor())->adapter->module_rst_ctr( SET_LOW );
}

void RADIO_nRESET_H() 
{
    if( mac_get_cursor() == NULL) return;
    obj_map_infor(mac_get_cursor())->adapter->module_rst_ctr( SET_HIG );
}

bool RADIO_Busy_Check()
{
    if( mac_get_cursor() == NULL) return true;
     return obj_map_infor(mac_get_cursor())->adapter->module_busy_check() ? true : false;
}


void SX126xReset( void )
{ 
    HAL_Delay( 10 );
    RADIO_nRESET_L();
    HAL_Delay( 20 );
    RADIO_nRESET_H();
    HAL_Delay( 10 );
}

void SX126xWaitOnBusy( void )
{
   while( RADIO_Busy_Check() );
}


void SX126xWakeup( void )
{
    RADIO_NSS_L();
  
    SpiInOut(RADIO_GET_STATUS);
    SpiInOut(0);
   
    RADIO_NSS_H(); 
    
    // Wait for chip to be ready.
    SX126xWaitOnBusy( );
}

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{

    SX126xCheckDeviceReady( );

    RADIO_NSS_L();

    SpiInOut(( uint8_t )command );

    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut(buffer[i] );
    }

    RADIO_NSS_H();
    
    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

void SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    RADIO_NSS_L();

    SpiInOut(( uint8_t )command );
    SpiInOut(0x00 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut(0 );
    }

    RADIO_NSS_H();

    SX126xWaitOnBusy( );
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    RADIO_NSS_L();
    
    SpiInOut(RADIO_WRITE_REGISTER );
    SpiInOut(( address & 0xFF00 ) >> 8 );
    SpiInOut( address & 0x00FF );
    
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut(buffer[i] );
    }
    
    
    RADIO_NSS_H();

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    RADIO_NSS_L();

    SpiInOut(RADIO_READ_REGISTER );
    SpiInOut(( address & 0xFF00 ) >> 8 );
    SpiInOut( address & 0x00FF );
    SpiInOut( 0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut(0 );
    }

    RADIO_NSS_H();

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    RADIO_NSS_L();
    
    SpiInOut( RADIO_WRITE_BUFFER );
    SpiInOut( offset );
    for( uint16_t i = 0; i < size; i++ )
    {
        SpiInOut( buffer[i] );
    }

    RADIO_NSS_H();

    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    RADIO_NSS_L();
    
    SpiInOut(  RADIO_READ_BUFFER );
    SpiInOut(  offset );
    SpiInOut(  0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( 0 );
    }

    RADIO_NSS_H();
    
    SX126xWaitOnBusy( );
}

void SX126xSetRfTxPower( int8_t power )
{
//SX126xSetTxParams( power, RADIO_RAMP_40_US );
  SX126xSetTxParams( power, RADIO_RAMP_200_US );
}

uint8_t SX126xGetPaSelect( uint32_t channel )
{
//    if( GpioRead( &DeviceSel ) == 1 )
//    {
//        return SX1261;
//    }
//    else
//    {
//        return SX1262;
//    }
  
  return SX1262;
}

void SX126xAntSwOn( void )
{
    //GpioInit( &AntPow, ANT_SWITCH_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
}

void SX126xAntSwOff( void )
{
   // GpioInit( &AntPow, ANT_SWITCH_POWER, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

bool SX126xCheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

