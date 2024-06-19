#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

// CRC types
#define CRC_TYPE_CCITT 0
#define CRC_TYPE_IBM 1
#define CRC_TYPE_

// Polynomial = X^16 + X^12 + X^5 + 1
#define POLYNOMIAL_CCITT 0x1021
// Polynomial = X^16 + X^15 + X^2 + 1
#define POLYNOMIAL_IBM 0x8005



// Seeds
#define CRC_IBM_SEED 0xFFFF
#define CRC_CCITT_SEED 0x1D0F

uint8_t RadioCompute_CRC8( uint8_t * buffer, uint16_t length );
uint16_t RadioCompute_CRC16( uint8_t *buffer, uint16_t length, uint8_t crcType );

#endif

