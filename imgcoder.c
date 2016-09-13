#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include "imgcoder.h"
#include "dct.h"
#include "huffman.h"

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

short* ic_flatten_block_data(short blockDataQuant[MB_SIZE][MB_SIZE][3])
{
    short* ret = (short*) malloc(sizeof(short) * MB_SIZE * MB_SIZE * 3);
    short* pos = ret;

    for(int rgb = 0; rgb < 3; rgb++)
    {
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                *pos++ = blockDataQuant[y][x][rgb];
            }
        }
    }

    return ret;
}

void ic_unflatten_block_data(short blockDataQuant[MB_SIZE][MB_SIZE][3], short* ret)
{
    for(int rgb = 0; rgb < 3; rgb++)
    {
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                blockDataQuant[y][x][rgb] = *ret++;
            }
        }
    }
}

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char out[MB_SIZE][MB_SIZE][3]);
void yuv_decode(float in[MB_SIZE][MB_SIZE][4], float out[MB_SIZE][MB_SIZE][4]);

void ic_compress_blocks(macroblock_t* blocks, short numBlocks, compressed_macroblock_t* cblocks)
{
    float blockDataDCT[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));  // Red Green Blue Unused

    for(int i = 0; i < numBlocks; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;

        yuv_encode(block->blockData, block->blockData);

        short blockDataQuant[MB_SIZE][MB_SIZE][3];
        dct_encode_block(block->blockData, blockDataDCT);
        dct_quantize_block(blockDataDCT, blockDataQuant);

        cblock->rle_data = ic_flatten_block_data(blockDataQuant);
        cblock->rle_size = sizeof(short) * MB_SIZE * MB_SIZE * 3;
    }
}

void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks, short numBlocks)
{
    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + numBlocks; cblock++)
    {
        free(cblock->rle_data);
    }

    free(cblocks);
}

short ic_get_num_blocks(const macroblock_t* blocks)
{
    const float rmsMin = 8.0;

    for(short i = 0; i < (MB_NUM_X * MB_NUM_Y) / 16; i++)
    {
        if(blocks[i].rms < rmsMin)
            return i;
    }

    return (MB_NUM_X * MB_NUM_Y) / 8;      // FIXME: make const
}

compressed_macroblock_t* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame, unsigned char* rmsView, short* num_blocks)
{
    macroblock_t* blocks = (macroblock_t*) malloc(MB_NUM_X * MB_NUM_Y * sizeof(macroblock_t));

    ic_fill_blocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), ic_sort_blocks);

    *num_blocks = ic_get_num_blocks(blocks);
    ic_show_rms(blocks, rmsView, *num_blocks);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) calloc(*num_blocks, sizeof(compressed_macroblock_t));
    ic_compress_blocks(blocks, *num_blocks, cblocks);

    free(blocks);

    return cblocks;
}

void ic_decode_image(const unsigned char* prevFrame, const compressed_macroblock_t* cblocks, const short numBlocks, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    for(const compressed_macroblock_t* block = cblocks; block < cblocks + numBlocks; block++)
    {
        short qdata[MB_SIZE][MB_SIZE][3];
        float data[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));
        float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

        assert(block->rle_size > 0);

        ic_unflatten_block_data(qdata, block->rle_data);
        dct_unquantize_block(qdata, data);
        dct_decode_block(data, pixels);

        yuv_decode(pixels, pixels);

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
