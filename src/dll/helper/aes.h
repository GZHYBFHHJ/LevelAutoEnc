#ifndef _ANTOENC_AES_H_
#define _ANTOENC_AES_H_

void aes_initkey256cbc(const char *key);

#define AES_MAX_BLOCK_LENGTH 32

int aes_encrypt(const unsigned char *input, int inputlen, unsigned char *output);

int aes_decrypt(const unsigned char *input, int inputlen, unsigned char *output);

#endif