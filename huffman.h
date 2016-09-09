#ifndef HUFFMAN_H_INCLUDED
#define HUFFMAN_H_INCLUDED

#include <stdbool.h>

#include "array.h"

typedef struct node_s
{
    unsigned int count;
    bool is_leaf;
    struct node_s* left;
    struct node_s* right;
    unsigned char val;
} node_t;

typedef struct
{
    unsigned short count;       // more than 256 blocks could theoretically be a problem
    unsigned char val;
} __attribute__((packed)) frequency_t;

#ifdef __cplusplus
extern "C" {
#endif

array_t* huffman_encode(const unsigned char* data, int datalen, unsigned short frequencies[256], unsigned int* bit_len);
array_t* huffman_decode(const unsigned char* data, int datalen, const unsigned short frequencies[256], unsigned int bit_len);

#ifdef __cplusplus
}
#endif


#endif // HUFFMAN_H_INCLUDED
