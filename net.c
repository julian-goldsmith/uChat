#include "net.h"
#include "array.h"
#include "huffman.h"
#include "lzw.h"
#include <SDL2/SDL.h>
#include <stdio.h>

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
        buffer = (unsigned char*) realloc(buffer, pos + 2 + sizeof(cblock->yout) + sizeof(cblock->uout) + sizeof(cblock->vout));

        *(buffer + pos++) = cblock->mb_x;
        *(buffer + pos++) = cblock->mb_y;

        memcpy(buffer + pos, cblock->yout, sizeof(cblock->yout));
        pos += sizeof(cblock->yout);

        memcpy(buffer + pos, cblock->uout, sizeof(cblock->uout));
        pos += sizeof(cblock->uout);

        memcpy(buffer + pos, cblock->vout, sizeof(cblock->vout));
        pos += sizeof(cblock->vout);
    }

    packet_header_t header;
    header.num_blocks = numBlocks;
    header.bit_len = pos;

    int lzstart = SDL_GetTicks();
    array_t* lz_compressed = array_create(2, pos);

    for(int i = 0; i < pos; i += 65536)
    {
        unsigned int tempcount = (i + 65536 > pos) ? pos - i : 65536;
        unsigned short* outlen = (unsigned short*) array_get_new(lz_compressed);
        unsigned int oldlen = lz_compressed->len;

        lz_encode(buffer + i, tempcount, lz_compressed);
        *outlen = lz_compressed->len - oldlen - 1;
    }

    printf("lz %i\n", SDL_GetTicks() - lzstart);

    lz_compressed->len *= 2;
    lz_compressed->item_size = 1;

    int huffstart = SDL_GetTicks();
    array_t* huff_compressed = huffman_encode(lz_compressed->base, lz_compressed->len, header.frequencies, &header.bit_len);
    printf("huff %i\n", SDL_GetTicks() - huffstart);

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

compressed_macroblock_t* net_deserialize_compressed_blocks(unsigned char* data, short* numBlocks)
{
    const packet_header_t* header = (packet_header_t*) data;

    assert(header->num_blocks >= 0);
    *numBlocks = header->num_blocks;

    array_t* unhuff = huffman_decode(data + sizeof(packet_header_t),
                        header->compressed_len, header->frequencies, header->bit_len);

    assert(unhuff->len % 2 == 0);
    unhuff->item_size = sizeof(short);
    unhuff->len /= 2;

    array_t* arr = array_create(1, unhuff->len * 2);

    unsigned int j = 0;
    while(j < unhuff->len)
    {
        unsigned short templen = *(unsigned short*) array_get(unhuff, j++) + 1;
        array_t* block = array_create_from_pointer(array_get(unhuff, j), 2, templen);

        array_t* temp = lz_decode(block);
        array_append_array(arr, temp);

        j += templen;
    }

    unsigned char* uncompressed = arr->base;

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * header->num_blocks);

    const unsigned char* bp = uncompressed;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + header->num_blocks; cblock++)
    {
        cblock->mb_x = *bp++;
        cblock->mb_y = *bp++;

        memcpy(cblock->yout, bp, sizeof(cblock->yout));
        bp += sizeof(cblock->yout);

        memcpy(cblock->uout, bp, sizeof(cblock->uout));
        bp += sizeof(cblock->uout);

        memcpy(cblock->vout, bp, sizeof(cblock->vout));
        bp += sizeof(cblock->vout);
    }

    free(unhuff);
    array_free(arr);

    return cblocks;
}
