#include "aes.h"

#include <openssl/evp.h>
#include <windows.h>

static unsigned char _key[32];
static unsigned char _iv[32] = { 0 };


void aes_initkey256cbc(const char *key) {
    CopyMemory(_key, key, 32);
}

#define AES_FAIL() do { EVP_CIPHER_CTX_free(ctx); return -1; } while (0) 

int aes_encrypt(const unsigned char *input, int inputlen, unsigned char *output) {
    EVP_CIPHER_CTX *ctx;

    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, _key, _iv)) AES_FAIL();

    int outlen;
    int len;
    if (!EVP_EncryptUpdate(ctx, output, &len, input, inputlen)) AES_FAIL();
    outlen = len;
    if (!EVP_EncryptFinal_ex(ctx, output + len, &len)) AES_FAIL();
    outlen += len;

    EVP_CIPHER_CTX_free(ctx);
    return outlen;
}

int aes_decrypt(const unsigned char *input, int inputlen, unsigned char *output) {
    EVP_CIPHER_CTX *ctx;

    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, _key, _iv)) AES_FAIL();

    int outlen;
    int len;
    if (!EVP_DecryptUpdate(ctx, output, &len, input, inputlen)) AES_FAIL();
    outlen = len;
    if (EVP_DecryptFinal_ex(ctx, output + len, &len) <= 0) AES_FAIL();
    outlen += len;

    EVP_CIPHER_CTX_free(ctx);
    return outlen;
}

