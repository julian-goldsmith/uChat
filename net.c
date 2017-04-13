#include "net.h"
#include "array.h"
#include "huffman.h"
#include "lzw.h"

typedef struct
{
    unsigned short magic;
    unsigned short num_blocks;
    unsigned short frequencies[256];
    unsigned int compressed_len;
    unsigned int bit_len;
} packet_header_t;

#include <SDL2/SDL.h>

unsigned char* net_serialize_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks)
{
    packet_header_t header;
    array_sint16_t* lz_enc_data;
    array_uint8_t* huff_enc_data;
    unsigned char* retval;

    int time1 = SDL_GetTicks();
    lz_enc_data = lz_encode((const unsigned char*) cblocks, sizeof(compressed_macroblock_t) * numBlocks);
    printf("lz in %u ticks\n", SDL_GetTicks() - time1);

    time1 = SDL_GetTicks();
    huff_enc_data = huffman_encode((const unsigned char*) lz_enc_data->base,
                                   lz_enc_data->len * 2,
                                   header.frequencies,
                                   &header.bit_len);
    printf("huffman in %u ticks\n", SDL_GetTicks() - time1);

    header.magic = 0x1234;
    header.compressed_len = huff_enc_data->len;
    header.num_blocks = numBlocks;
    *totalSize = sizeof(packet_header_t) + huff_enc_data->len;

    retval = (unsigned char*) malloc(*totalSize);
    memcpy(retval, &header, sizeof(packet_header_t));
    memcpy(retval + sizeof(packet_header_t), huff_enc_data->base, huff_enc_data->len);

    array_sint16_free(lz_enc_data);
    array_uint8_free(huff_enc_data);

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(unsigned char* data, short* numBlocks)
{
    compressed_macroblock_t* cblocks;
    array_uint8_t* huff_dec_data, *lz_dec_data;
    array_sint16_t* huff_dec_converted16;
    packet_header_t* header = (packet_header_t*) data;

    assert(header->magic == 0x1234);

    huff_dec_data = huffman_decode(data + sizeof(packet_header_t),
                                   header->compressed_len,
                                   header->frequencies,
                                   header->bit_len);
    huff_dec_converted16 = array_sint16_create_from_pointer((short*) huff_dec_data->base, huff_dec_data->len / 2);

    lz_dec_data = lz_decode(huff_dec_converted16);
    cblocks = (compressed_macroblock_t*) lz_dec_data->base;

    free(huff_dec_converted16);
    array_uint8_free(huff_dec_data);
    free(lz_dec_data);

    *numBlocks = header->num_blocks;

    return cblocks;
}
