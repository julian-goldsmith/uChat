#include "net.h"
#include "array.h"
#include "huffman.h"

typedef struct
{
    unsigned short num_blocks;
    unsigned short frequencies[256];
    unsigned int compressed_len;
} packet_header_t;

unsigned char* net_serialize_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks)
{
    unsigned char* buffer = (unsigned char*) malloc(sizeof(compressed_macroblock_t));
    int pos = 0;

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

    packet_header_t header;
    header.num_blocks = numBlocks;

    array_t* compressed = huffman_encode(buffer, pos, header.frequencies);
    header.compressed_len = compressed->len;

    unsigned char* retval = (unsigned char*) malloc(sizeof(packet_header_t) + header.compressed_len);
    memcpy(retval, &header, sizeof(packet_header_t));
    memcpy(retval + sizeof(packet_header_t), compressed->base, packet_header_t.compressed_len);

    free(buffer);
    array_free(compressed);

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(const unsigned char* data, short* numBlocks)
{
    packet_header_t* header = (packet_header_t*) data;

    assert(header.num_blocks >= 0);
    *numBlocks = header.num_blocks;

    int outdatalen;
    unsigned char* uncompressed = huffman_decode(data, header.compressed_len, &outdatalen);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * header.num_blocks);

    const unsigned char* bp = uncompressed;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + header.num_blocks; cblock++)
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
