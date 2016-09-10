#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "imgcoder.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* net_serialize_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks);
compressed_macroblock_t* net_deserialize_compressed_blocks(unsigned char* data, short* numBlocks);

#ifdef __cplusplus
}
#endif

#endif // NET_H_INCLUDED
