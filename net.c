#include "net.h"
#include "array.h"
#include "huffman.h"

unsigned char* net_serialize_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks)
{
    unsigned char* buffer = (unsigned char*) malloc(sizeof(compressed_macroblock_t));
    int pos = 0;

    *(short*) (buffer + pos) = numBlocks;
    pos += sizeof(short);

    for(const compressed_macroblock_t* cblock = cblocks; cblock < cblocks + numBlocks; cblock++)
    {
        // FIXME: don't alloc so often
        buffer = (unsigned char*) realloc(buffer, pos + 4 + cblock->rle_size * 2);

        *(buffer + pos++) = cblock->mb_x;
        *(buffer + pos++) = cblock->mb_y;

        *(short*) (buffer + pos) = cblock->rle_size;
        pos += 2;

        memcpy(buffer + pos, cblock->rle_data, cblock->rle_size * 2);
        pos += cblock->rle_size * 2;
    }

    array_t* compressed = huffman_encode(buffer, pos);
    *totalSize = compressed->len;

    unsigned char* retval = compressed->base;
    free(buffer);
    free(compressed);

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(const unsigned char* data, const int datalen, short* numBlocks)
{
    int outdatalen;
    unsigned char* uncompressed = huffman_decode(data, datalen, &outdatalen);

    *numBlocks = *(short*) uncompressed;

    assert(*numBlocks >= 0);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * *numBlocks);

    const unsigned char* bp = uncompressed + sizeof(short);

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + *numBlocks; cblock++)
    {
        cblock->mb_x = *bp++;
        cblock->mb_y = *bp++;

        cblock->rle_size = *(short*) bp;
        bp += 2;

        cblock->rle_data = (short*) malloc(cblock->rle_size * 2);
        memcpy(cblock->rle_data, bp, cblock->rle_size * 2);
        bp += cblock->rle_size * 2;
    }

    free(uncompressed);

    return cblocks;
}
