#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include "imgcoder.h"
#include "dct.h"
#include "huffman.h"
#include "yuv.h"
#include "bwt.h"

int ic_sort_blocks(const void* val1, const void* val2)
{
    const macroblock_t* mb1 = (const macroblock_t*) val1;
    const macroblock_t* mb2 = (const macroblock_t*) val2;

    if(mb1->rms < mb2->rms)
        return 1;
    else if(mb1->rms > mb2->rms)
        return -1;
    else
        return 0;
}

void ic_calculate_block_rms(macroblock_t* block, const unsigned char* prevFrame)
{
    int blockAvg[3];

    blockAvg[0] = 0;
    blockAvg[1] = 0;
    blockAvg[2] = 0;

    for(int blx = 0; blx < MB_SIZE; blx++)
    {
        for(int bly = 0; bly < MB_SIZE; bly++)
        {
            int frameBase = 3 * ((block->mb_y * MB_SIZE + bly) * 640 + (block->mb_x * MB_SIZE + blx));

            char dr = block->blockData[blx][bly][0] - prevFrame[frameBase];
            char dg = block->blockData[blx][bly][1] - prevFrame[frameBase + 1];
            char db = block->blockData[blx][bly][2] - prevFrame[frameBase + 2];

            blockAvg[0] += dr * dr;
            blockAvg[1] += dg * dg;
            blockAvg[2] += db * db;
        }
    }

    block->rms = sqrtf((float) blockAvg[0] / (MB_SIZE * MB_SIZE)) + sqrtf((float) blockAvg[1] / (MB_SIZE * MB_SIZE)) +
                 sqrtf((float) blockAvg[2] / (MB_SIZE * MB_SIZE));
}

void ic_build_block(macroblock_t* block, const unsigned char* imgIn)
{
    for(int blx = 0; blx < MB_SIZE; blx++)
    {
        for(int bly = 0; bly < MB_SIZE; bly++)
        {
            int frameBase = 3 * ((block->mb_y * MB_SIZE + bly) * 640 + (block->mb_x * MB_SIZE + blx));
            block->blockData[blx][bly][0] = imgIn[frameBase];
            block->blockData[blx][bly][1] = imgIn[frameBase + 1];
            block->blockData[blx][bly][2] = imgIn[frameBase + 2];
        }
    }
}

void ic_fill_blocks(macroblock_t* blocks, const unsigned char* imgIn, const unsigned char* prevFrame)
{
    for(int x = 0; x < MB_NUM_X; x++)
    {
        for(int y = 0; y < MB_NUM_Y; y++)
        {
            macroblock_t* block = blocks + y * MB_NUM_X + x;
            block->mb_x = x;
            block->mb_y = y;
            block->rms = 0.0;
            ic_build_block(block, imgIn);

            ic_calculate_block_rms(block, prevFrame);
        }
    }
}

void ic_show_rms(const macroblock_t* blocks, unsigned char* rmsView, short numBlocks)
{
    float rmsMax = 0.0;
    memset(rmsView, 0, 640*480*3);

    for(const macroblock_t* block = blocks; block < blocks + (MB_NUM_X * MB_NUM_Y); block++)
    {
        if(block->rms > rmsMax)
            rmsMax = block->rms;
    }

    int bnum = 0;
    for(const macroblock_t* block = blocks; block < blocks + (MB_NUM_X * MB_NUM_Y); block++)
    {
        float val = (block->rms / rmsMax);

        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);

                if(bnum < numBlocks)
                {
                    rmsView[frameBase] = 0;
                    rmsView[frameBase + 1] = 256 * val;
                    rmsView[frameBase + 2] = 0;
                }
                else
                {
                    rmsView[frameBase] = 256 * val;
                    rmsView[frameBase + 1] = 256 * val;
                    rmsView[frameBase + 2] = 256 * val;
                }
            }
        }

        bnum++;
    }
}

static const int zigzag_indices[MB_NUM_X][MB_NUM_Y] =
{
    { 0x00, 0x01, 0x05, 0x06, 0x0e, 0x0f, 0x1b, 0x2b, 0x2c, 0x40, 0x41, 0x59, 0x5a, 0x76, 0x77, 0x95 },
    { 0x02, 0x04, 0x07, 0x0d, 0x10, 0x1a, 0x1c, 0x2a, 0x2d, 0x3f, 0x42, 0x58, 0x5b, 0x75, 0x78, 0x96 },
    { 0x03, 0x08, 0x0c, 0x11, 0x19, 0x1d, 0x29, 0x2e, 0x3e, 0x43, 0x57, 0x5c, 0x74, 0x79, 0x94, 0x97 },
    { 0x09, 0x0b, 0x12, 0x18, 0x1e, 0x28, 0x2f, 0x3d, 0x44, 0x56, 0x5d, 0x73, 0x7a, 0x93, 0x98, 0xb1 },
    { 0x0a, 0x13, 0x17, 0x1f, 0x27, 0x30, 0x3c, 0x45, 0x55, 0x5e, 0x72, 0x7b, 0x92, 0x99, 0xb0, 0xb2 },
    { 0x14, 0x16, 0x20, 0x26, 0x31, 0x3b, 0x46, 0x54, 0x5f, 0x71, 0x7c, 0x91, 0x9a, 0xaf, 0xb3, 0xc8 },
    { 0x15, 0x21, 0x25, 0x32, 0x3a, 0x47, 0x53, 0x60, 0x70, 0x7d, 0x90, 0x9b, 0xae, 0xb4, 0xc7, 0xc9 },
    { 0x22, 0x24, 0x33, 0x39, 0x48, 0x52, 0x61, 0x6f, 0x7e, 0x8f, 0x9c, 0xad, 0xb5, 0xc6, 0xca, 0xdb },
    { 0x23, 0x34, 0x38, 0x49, 0x51, 0x62, 0x6e, 0x7f, 0x8e, 0x9d, 0xac, 0xb6, 0xc5, 0xcb, 0xda, 0xdc },
    { 0x35, 0x37, 0x4a, 0x50, 0x63, 0x6d, 0x80, 0x8d, 0x9e, 0xab, 0xb7, 0xc4, 0xcc, 0xd9, 0xde, 0xdd },
    { 0x36, 0x4b, 0x4f, 0x64, 0x6c, 0x81, 0x8c, 0x9f, 0xaa, 0xb8, 0xc3, 0xcd, 0xd8, 0xdf, 0xea, 0xeb },
    { 0x4c, 0x4e, 0x65, 0x6b, 0x82, 0x8b, 0xa0, 0xa9, 0xb9, 0xc2, 0xce, 0xd7, 0xe0, 0xe9, 0xed, 0xec },
    { 0x4d, 0x66, 0x6a, 0x83, 0x8a, 0xa1, 0xa8, 0xba, 0xc1, 0xcf, 0xd6, 0xe1, 0xe8, 0xee, 0xf5, 0xf6 },
    { 0x67, 0x69, 0x84, 0x89, 0xa2, 0xa7, 0xbb, 0xc0, 0xd0, 0xd5, 0xe2, 0xe7, 0xef, 0xf4, 0xf8, 0xf7 },
    { 0x68, 0x85, 0x88, 0xa3, 0xa6, 0xbc, 0xbf, 0xd1, 0xd4, 0xe3, 0xe6, 0xf0, 0xf3, 0xf9, 0xfc, 0xfd },
    { 0x86, 0x87, 0xa4, 0xa5, 0xbd, 0xbe, 0xd2, 0xd3, 0xe4, 0xe5, 0xf1, 0xf2, 0xfa, 0xfb, 0xff, 0xfe },
};

void ic_zigzag_array(const short invals[MB_SIZE][MB_SIZE], short outvals[MB_SIZE][MB_SIZE])
{
    for(int x = 0; x < MB_SIZE; x++)
    for(int y = 0; y < MB_SIZE; y++)
    {
        short index = zigzag_indices[x][y];
        outvals[index >> 4][index & 0xf] = invals[x][y];
    }
}

void ic_unzigzag_array(const short invals[MB_SIZE][MB_SIZE], short outvals[MB_SIZE][MB_SIZE])
{
    for(int x = 0; x < MB_SIZE; x++)
    for(int y = 0; y < MB_SIZE; y++)
    {
        short index = zigzag_indices[x][y];
        outvals[x][y] = invals[index >> 4][index & 0xf];
    }
}

void ic_compress_blocks(macroblock_t* blocks, short numBlocks, compressed_macroblock_t* cblocks)
{
    int bwttime = 0;
    for(int i = 0; i < numBlocks; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        cblock->magic = 0x1234;
        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;

        unsigned char yout[MB_SIZE][MB_SIZE];
        unsigned char uout[4][4];
        unsigned char vout[4][4];

        yuv_encode(block->blockData, yout, uout, vout);

        short tempdyout[MB_SIZE][MB_SIZE];
        dct_encode_block(yout, uout, vout, tempdyout, cblock->uout, cblock->vout);
        ic_zigzag_array((const short(*)[16]) tempdyout, cblock->yout);

        int time1 = SDL_GetTicks();
        short temp[256];
        bwt_encode(&cblock->yout[0][0], temp, cblock->indexlist);
        bwttime += (SDL_GetTicks() - time1);

        memcpy(&cblock->yout[0][0], temp, 512);
    }
    printf("compress %hu blocks in avg %f ms, total %u ms\n", numBlocks, (float) bwttime / numBlocks, bwttime);
}

void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks, short numBlocks)
{
    free(cblocks);
}

short ic_get_num_blocks(const macroblock_t* blocks)
{
    const float rmsMin = 32.0;

    for(short i = 0; i < (MB_NUM_X * MB_NUM_Y) / 16; i++)
    {
        if(blocks[i].rms < rmsMin)
            return i;
    }

    return (MB_NUM_X * MB_NUM_Y) / 4;      // FIXME: make const
}

compressed_macroblock_t* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame,
                                         unsigned char* rmsView, short* num_blocks)
{
    macroblock_t blocks[MB_NUM_X * MB_NUM_Y];

    ic_fill_blocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), ic_sort_blocks);

    *num_blocks = ic_get_num_blocks(blocks);
    ic_show_rms(blocks, rmsView, *num_blocks);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(*num_blocks * sizeof(compressed_macroblock_t));
    ic_compress_blocks(blocks, *num_blocks, cblocks);

    return cblocks;
}

void ic_decode_image(const unsigned char* prevFrame, const compressed_macroblock_t* cblocks, const short numBlocks, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    for(const compressed_macroblock_t* block = cblocks; block < cblocks + numBlocks; block++)
    {
        assert(block->magic == 0x1234);

        unsigned char pixels[MB_SIZE][MB_SIZE][3];

        unsigned char yout[MB_SIZE][MB_SIZE];
        unsigned char uout[4][4];
        unsigned char vout[4][4];

        short asdf[16][16];
        bwt_decode(block->yout, block->indexlist, &asdf[0]);

        short tempdyout[MB_SIZE][MB_SIZE];
        ic_unzigzag_array(asdf, tempdyout);

        dct_decode_block((const short(*)[MB_SIZE]) tempdyout, block->uout, block->vout,
                         yout, uout, vout);

        yuv_decode((const unsigned char(*)[16]) yout, (const unsigned char(*)[4]) uout,
                   (const unsigned char(*)[4]) vout, pixels);

        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);
                frameOut[frameBase] = pixels[x][y][0];
                frameOut[frameBase + 1] = pixels[x][y][1];
                frameOut[frameBase + 2] = pixels[x][y][2];
            }
        }
    }
}
