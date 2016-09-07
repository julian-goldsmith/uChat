#include "net.h"
#include "array.h"
#include "huffman.h"

typedef struct
{
    unsigned short num_blocks;
    unsigned short frequencies[256];
    unsigned int compressed_len;
    unsigned int bit_len;
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

    array_t* compressed = huffman_encode(buffer, pos, header.frequencies, &header.bit_len);
    header.compressed_len = compressed->len;

    *totalSize = sizeof(packet_header_t) + header.compressed_len;

    unsigned char* retval = (unsigned char*) malloc(*totalSize);
    memcpy(retval, &header, sizeof(packet_header_t));
    memcpy(retval + sizeof(packet_header_t), compressed->base, header.compressed_len);

    //printf("%i, %i, %i, %i, %i\n", header.compressed_len, pos, header.bit_len, header.frequencies[0], header.frequencies[255]);

    free(buffer);
    array_free(compressed);

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(const unsigned char* data, short* numBlocks)
{
    const packet_header_t* header = (packet_header_t*) data;

    assert(header->num_blocks >= 0);
    *numBlocks = header->num_blocks;

    int outdatalen;
    unsigned char* uncompressed = huffman_decode(data + sizeof(packet_header_t),
                                                 header->compressed_len, &outdatalen,
                                                 header->frequencies, header->bit_len);

    //printf("%i, %i, %i, %i, %i\n", header->compressed_len, outdatalen, header->bit_len, header->frequencies[0], header->frequencies[255]);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * header->num_blocks);

    const unsigned char* bp = uncompressed;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + header->num_blocks; cblock++)
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
