#ifndef HUFFMAN_H_INCLUDED
#define HUFFMAN_H_INCLUDED

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

array_t* huffman_encode(const unsigned char* data, int datalen);
unsigned char* huffman_decode(const unsigned char* data, int datalen, int* outdatalen);

#ifdef __cplusplus
}
#endif


#endif // HUFFMAN_H_INCLUDED
