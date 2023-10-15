#ifndef _ANTOENC_7Z_H_
#define _ANTOENC_7Z_H_


int sevenzip_available();


typedef struct SEVENZIP_RESULT SEVENZIP_RESULT;

SEVENZIP_RESULT *sevenzip_compress(const unsigned char *input, int inputlen);

SEVENZIP_RESULT *sevenzip_decompress(const unsigned char *input, int inputlen);

int sevenzip_res_length(SEVENZIP_RESULT *res);

int sevenzip_res_read(SEVENZIP_RESULT *res, unsigned char *output);

void sevenzip_res_free(SEVENZIP_RESULT *res);

#endif