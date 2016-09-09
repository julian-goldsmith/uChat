#include "net.h"
#include "array.h"
#include "huffman.h"
#include "lzw.h"
#include <SDL2/SDL.h>

array_t* lz_encode(unsigned char* file_data, int file_len);
array_t* lz_decode(array_t* enc_data);

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

        memcpy(buffer + pos, cblock->rle_data, cblock->rle_size);
        pos += cblock->rle_size;
    }

    packet_header_t header;
    header.num_blocks = numBlocks;
    header.bit_len = pos;

    int lzStart = SDL_GetTicks();
    array_t* lz_compressed = lz_encode(buffer, pos);
    printf("comp: %i\n", SDL_GetTicks() - lzStart);

    lz_compressed->len *= 2;
    lz_compressed->item_size = 1;

    array_t* huff_compressed = huffman_encode(lz_compressed->base, lz_compressed->len, header.frequencies, &header.bit_len);

    header.compressed_len = huff_compressed->len;

    *totalSize = sizeof(packet_header_t) + header.compressed_len;

    unsigned char* retval = (unsigned char*) malloc(*totalSize);
    memcpy(retval, &header, sizeof(packet_header_t));
    memcpy(retval + sizeof(packet_header_t), huff_compressed->base, header.compressed_len);

    free(buffer);
    array_free(lz_compressed);
    array_free(huff_compressed);

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(const unsigned char* data, short* numBlocks)
{
    const packet_header_t* header = (packet_header_t*) data;

    assert(header->num_blocks >= 0);
    *numBlocks = header->num_blocks;

    array_t* unhuff = huffman_decode(data + sizeof(packet_header_t),
                        header->compressed_len, header->frequencies, header->bit_len);

    assert(unhuff->len % 2 == 0);
    unhuff->item_size = sizeof(short);
    unhuff->len /= 2;

    array_t* arr = lz_decode(unhuff);

    unsigned char* uncompressed = arr->base;

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * header->num_blocks);

    const unsigned char* bp = uncompressed;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + header->num_blocks; cblock++)
    {
        cblock->mb_x = *bp++;
        cblock->mb_y = *bp++;

        cblock->rle_size = *(short*) bp;
        bp += 2;

        cblock->rle_data = (short*) malloc(cblock->rle_size);
        memcpy(cblock->rle_data, bp, cblock->rle_size);
        bp += cblock->rle_size;
    }

    free(unhuff);
    array_free(arr);

    return cblocks;
}