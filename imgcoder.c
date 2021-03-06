#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include "imgcoder.h"
#include "dct.h"
#include "huffman.h"
#include "yuv.h"

static int ic_sort_blocks(const void* val1, const void* val2)
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

static void ic_calculate_block_rms(macroblock_t* block, const unsigned char* prevFrame)
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
    macroblock_t* block = blocks;
    for(int x = 0; x < MB_NUM_X; x++)
    {
        for(int y = 0; y < MB_NUM_Y; y++)
        {
            block->mb_x = x;
            block->mb_y = y;
            ic_build_block(block, imgIn);

            ic_calculate_block_rms(block, prevFrame);

            block++;
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

void ic_compress_blocks(macroblock_t* blocks, short numBlocks, compressed_macroblock_t* cblocks)
{
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
        dct_encode_block(yout, uout, vout, cblock->yout, cblock->uout, cblock->vout);
    }
}

void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks, short numBlocks)
{
    free(cblocks);
}

short ic_get_num_blocks(const macroblock_t* blocks)
{
    const float rmsMin = 16.0;
    const short blockDiv = 2;

    for(short i = 0; i < (MB_NUM_X * MB_NUM_Y) / blockDiv; i++)
    {
        if(blocks[i].rms < rmsMin)
            return i;
    }

    return (MB_NUM_X * MB_NUM_Y) / blockDiv;
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

        dct_decode_block(block->yout, block->uout, block->vout,
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
