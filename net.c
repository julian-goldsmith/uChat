#include "net.h"
#include "array.h"
#include "huffman.h"
#include "lzw.h"
#include <SDL2/SDL.h>
#include <stdio.h>

typedef struct
{
    unsigned short magic;
    unsigned short num_blocks;
    unsigned short frequencies[256];
    unsigned int compressed_len;
    unsigned int bit_len;
} packet_header_t;

short* bwt_encode(const short* inarr, int* posp);
short* bwt_decode(short* inarr, int inpos);

unsigned char* net_serialize_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks)
{
    const compressed_macroblock_t* cblock;
    unsigned char* buffer = (unsigned char*) malloc(numBlocks * (2 + 2 + sizeof(cblock->yout) + sizeof(cblock->uout) + sizeof(cblock->vout)));
    int pos = 0;

    for(cblock = cblocks; cblock < cblocks + numBlocks; cblock++)
    {
        *(short*)(buffer + pos) = cblock->magic;
        pos += 2;

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

    header.magic = 0x1234;

    array_sint16_t* lz_enc_data = array_sint16_create(1024);
    lz_encode(buffer, pos, lz_enc_data);

    array_uint8_t* huff_enc_data = huffman_encode((const unsigned char*) lz_enc_data->base,
                                                  lz_enc_data->len * 2,
                                                  header.frequencies,
                                                  &header.bit_len);

    header.compressed_len = huff_enc_data->len;
    header.num_blocks = numBlocks;

    unsigned char* retval = (unsigned char*) malloc(sizeof(packet_header_t) + huff_enc_data->len);
    memcpy(retval, &header, sizeof(packet_header_t));
    memcpy(retval + sizeof(packet_header_t), huff_enc_data->base, huff_enc_data->len);

    *totalSize = sizeof(packet_header_t) + huff_enc_data->len;

    return retval;
}

compressed_macroblock_t* net_deserialize_compressed_blocks(unsigned char* data, short* numBlocks)
{
    packet_header_t* header = (packet_header_t*) data;

    assert(header->magic == 0x1234);

    array_uint8_t* huff_dec_data = huffman_decode(data + sizeof(packet_header_t),
                                                  header->compressed_len,
                                                  header->frequencies,
                                                  header->bit_len);
    array_sint16_t* huff_dec_converted16 = array_sint16_create_from_pointer((short*) huff_dec_data->base, huff_dec_data->len / 2);

    array_uint8_t* lz_dec_data = lz_decode(huff_dec_converted16);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * header->num_blocks);

    const unsigned char* bp = lz_dec_data->base;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + header->num_blocks; cblock++)
    {
        cblock->magic = *(short*) bp;
        bp += 2;

        cblock->mb_x = *bp++;
        cblock->mb_y = *bp++;

        memcpy(cblock->yout, bp, sizeof(cblock->yout));
        bp += sizeof(cblock->yout);

        memcpy(cblock->uout, bp, sizeof(cblock->uout));
        bp += sizeof(cblock->uout);

        memcpy(cblock->vout, bp, sizeof(cblock->vout));
        bp += sizeof(cblock->vout);
    }

    *numBlocks = header->num_blocks;

    //free(unhuff);
    //array_uint8_free(arr);

    return cblocks;
}
