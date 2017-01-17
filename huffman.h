#ifndef HUFFMAN_H_INCLUDED
#define HUFFMAN_H_INCLUDED

#include <stdbool.h>

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

array_uint8_t* huffman_encode(const unsigned char* data, int datalen, unsigned short frequencies[256], unsigned int* bit_len);
array_uint8_t* huffman_decode(unsigned char* data, int datalen, const unsigned short frequencies[256], unsigned int bit_len);

#ifdef __cplusplus
}
#endif


#endif // HUFFMAN_H_INCLUDED
