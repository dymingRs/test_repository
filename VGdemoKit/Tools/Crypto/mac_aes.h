#ifndef  _MAC_AES_H_
#define _MAC_AES_H_

bool mac_aes_encrypt( uint8_t *buffer, int16_t size, uint32_t address, uint8_t type,  uint8_t frameCounter, const uint8_t key[16] );
bool mac_aes_decrypt( uint8_t *sbuffer, int16_t size, const uint8_t key[16] );

#endif