#include <stdbool.h>
#include <string.h>

#include "lorawan_aes.h"
#include "mac.h"


static void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    while( size-- )
    {
        *dst++ = value;
    }
}

static bool mac_element_encrypt(uint8_t *buffer, uint8_t *encBuffer, uint8_t size,const uint8_t key[16])
{ 
  if( ( buffer == NULL ) || ( encBuffer == NULL ) )
  {
    return false;
  }
  
  /* Check if the size is divisible by 16 */
  if( ( size % 16 ) != 0 )
  {
    return false;
  }
  
  uint8_t block = 0;
  lorawan_aes_context aesContext;
  
  memset1( aesContext.ksch, '\0', 240 );
  lorawan_aes_set_key( key, 16, &aesContext );
  
  while( size != 0 )
  {
    lorawan_aes_encrypt( &buffer[block], &encBuffer[block], &aesContext );
    block = block + 16;
    size  = size - 16;
  }
  
  return true;
}


bool mac_aes_encrypt( uint8_t *buffer, int16_t size, uint32_t address, uint8_t type,  uint8_t frameCounter, const uint8_t key[16] )
{
  if( ( buffer == NULL ) || ( size == 0) )
  {
    return false;
  }
  if( size <16 )
  {
    return false;
  }
  
  uint8_t bufferIndex = 0;
  uint16_t ctr = 1;
  uint8_t sBlock[16] = { 0 };
  uint8_t aBlock[16] = { 0 };
  
  aBlock[0] = 0x01;
  aBlock[5] = type;
  aBlock[6] = address & 0xFF;
  aBlock[7] = ( address >> 8 ) & 0xFF;
  aBlock[8] = ( address >> 16 ) & 0xFF;
  aBlock[9] = ( address >> 24 ) & 0xFF;
  aBlock[10] = frameCounter & 0xFF;
  aBlock[11] = ( ( frameCounter ^ 1 ) >> 8  ) & 0xFF;
  aBlock[12] = ( ( frameCounter ^ 2 ) >> 16 ) & 0xFF;
  aBlock[13] = ( ( frameCounter ^ 3 ) >> 24 ) & 0xFF;
  
  if( mac_element_encrypt( buffer, sBlock, 16, key ) == false )
  {
    return false;
  }
  
  memcpy( buffer, sBlock, 16);
  bufferIndex += 16;
  size -= 16;
  
  while( size > 0 )
  {
    aBlock[15] = ctr & 0xFF;
    ctr++;
    
    if( mac_element_encrypt( aBlock, sBlock, 16, key ) == false )
    {
      return false;
    }
    
    for( uint8_t i = 0; i < ( ( size > 16 ) ? 16 : size ); i++ )
    {
      buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
    }
    size -= 16;
    bufferIndex += 16;
  }
  
  return true;
}


bool mac_aes_decrypt( uint8_t *sbuffer, int16_t size, const uint8_t key[16] )
{
  if( (sbuffer == NULL) || (size==NULL) )
  {
    return false;
  }
  if( size <16 )
  {
    return false;
  }
  
  uint8_t block = 0;
  uint16_t ctr = 1;
  uint8_t sBlock[16] = { 0 };
  uint8_t aBlock[16] = { 0 };

  uint32_t address;
  uint8_t type;
  uint8_t frameCounter;
  lorawan_aes_context aesContext;
  
  memset1( aesContext.ksch, '\0', 240 );
  lorawan_aes_set_key( key, 16, &aesContext );//set aes key
  lorawan_aes_decrypt( sbuffer, aBlock, &aesContext );//decrypt 
  
  memcpy( sbuffer, aBlock, 16);

  type = aBlock[TYPE_INDEX];
  address = aBlock[DEV_ID_INDEX+3];
  address <<= 8;
  address |= aBlock[DEV_ID_INDEX+2];
  address <<= 8;
  address |= aBlock[DEV_ID_INDEX+1];
  address <<=8 ;
  address |= aBlock[DEV_ID_INDEX+0];
  frameCounter = aBlock[SEQ_INDEX];
  
  memset1(aBlock ,0,16);
  
  aBlock[0] = 0x01;
  aBlock[5] = type;
  aBlock[6] = address & 0xFF;
  aBlock[7] = ( address >> 8 ) & 0xFF;
  aBlock[8] = ( address >> 16 ) & 0xFF;
  aBlock[9] = ( address >> 24 ) & 0xFF;
  aBlock[10] = frameCounter & 0xFF;
  aBlock[11] = ( ( frameCounter ^ 1 ) >> 8 ) & 0xFF;
  aBlock[12] = ( ( frameCounter ^ 2 ) >> 16 ) & 0xFF;
  aBlock[13] = ( ( frameCounter ^ 3 ) >> 24 ) & 0xFF;
  
  block = block + 16;
  size  = size - 16;
    
  while( size > 0 )
  {
    aBlock[15] = ctr & 0xFF;
    ctr++;
    
    for( uint8_t i = 0; i < ( ( size > 16 ) ? 16 : size ); i++ )
    {
      if( mac_element_encrypt( aBlock, sBlock, 16, key ) == false )
      {
        return false;
      }
      
      sbuffer[block + i] = sbuffer[block + i] ^ sBlock[i];
    }
    
    block = block + 16;
    size  = size - 16;
  }

  return true;
}