#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include "imgcoder.h"
#include "dct.h"
#include "rle.h"

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
    float blockAvg[3];

    blockAvg[0] = 0.0;
    blockAvg[1] = 0.0;
    blockAvg[2] = 0.0;

    for(int blx = 0; blx < MB_SIZE; blx++)
    {
        for(int bly = 0; bly < MB_SIZE; bly++)
        {
            int frameBase = 3 * ((block->mb_y * MB_SIZE + bly) * 640 + (block->mb_x * MB_SIZE + blx));

            float dr = block->blockData[blx][bly][0] - prevFrame[frameBase];
            float dg = block->blockData[blx][bly][1] - prevFrame[frameBase + 1];
            float db = block->blockData[blx][bly][2] - prevFrame[frameBase + 2];

            blockAvg[0] += dr * dr;
            blockAvg[1] += dg * dg;
            blockAvg[2] += db * db;
        }
    }

    block->rms = sqrt(blockAvg[0] / (MB_SIZE * MB_SIZE)) + sqrt(blockAvg[1] / (MB_SIZE * MB_SIZE)) +
                 sqrt(blockAvg[2] / (MB_SIZE * MB_SIZE));
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

void ic_show_rms(const macroblock_t* blocks, unsigned char* rmsView)
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
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);

                if(bnum < NUMBLOCKS)
                {
                    rmsView[frameBase] = 0;
                    rmsView[frameBase + 1] = 256 * (block->rms / rmsMax);
                    rmsView[frameBase + 2] = 0;
                }
                else
                {
                    rmsView[frameBase] = 256 * (block->rms / rmsMax);
                    rmsView[frameBase + 1] = 256 * (block->rms / rmsMax);
                    rmsView[frameBase + 2] = 256 * (block->rms / rmsMax);
                }
            }
        }

        bnum++;
    }
}

void ic_compress_blocks(macroblock_t* blocks, compressed_macroblock_t* cblocks)
{
    float blockDataDCT[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));  // Red Green Blue Unused
    short blockDataQuant[MB_SIZE][MB_SIZE][3];

    for(int i = 0; i < NUMBLOCKS; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        dct_encode_block(block->blockData, blockDataDCT);
        dct_quantize_block(blockDataDCT, blockDataQuant);

        int rleSize;
        short* rleData = rle_encode_block(blockDataQuant, &rleSize);

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;
        cblock->rleData = rleData;
        cblock->rleSize = rleSize;
    }
}

unsigned char* ic_stream_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize)
{
    int rleTotal = 0;

    for(const compressed_macroblock_t* cblock = cblocks; cblock < cblocks + NUMBLOCKS; cblock++)
    {
        rleTotal += cblock->rleSize;
    }

    *totalSize = (rleTotal * sizeof(short)) + NUMBLOCKS * (sizeof(compressed_macroblock_t) - sizeof(short*));

    unsigned char* buffer = (unsigned char*) malloc(*totalSize);
    unsigned char* bp = buffer;

    for(const compressed_macroblock_t* cblock = cblocks; cblock < cblocks + NUMBLOCKS; cblock++)
    {
        *bp++ = cblock->mb_x;
        *bp++ = cblock->mb_y;

        memcpy(bp, &cblock->rleSize, sizeof(cblock->rleSize));
        bp += sizeof(cblock->rleSize);

        memcpy(bp, cblock->rleData, cblock->rleSize * sizeof(short));
        bp += cblock->rleSize * sizeof(short);
    }

    return buffer;
}

compressed_macroblock_t* ic_unstream_compressed_blocks(const unsigned char* data)
{
    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * NUMBLOCKS);  // FIXME: do dynamically

    const unsigned char* bp = data;

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + NUMBLOCKS; cblock++)
    {
        cblock->mb_x = *bp++;
        cblock->mb_y = *bp++;

        memcpy(&cblock->rleSize, bp, sizeof(cblock->rleSize));
        bp += sizeof(cblock->rleSize);

        cblock->rleData = (short*) malloc(sizeof(short) * cblock->rleSize);
        memcpy(cblock->rleData, bp, sizeof(short) * cblock->rleSize);
        bp += sizeof(short) * cblock->rleSize;
    }

    return cblocks;
}

void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks)
{
    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + NUMBLOCKS; cblock++)
    {
        free(cblock->rleData);
    }

    free(cblocks);
}

unsigned char* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame, unsigned char* rmsView, int* totalSize)
{
    macroblock_t* blocks = (macroblock_t*) malloc(MB_NUM_X * MB_NUM_Y * sizeof(macroblock_t));

    ic_fill_blocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), ic_sort_blocks);

    ic_show_rms(blocks, rmsView);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) calloc(NUMBLOCKS, sizeof(compressed_macroblock_t));
    ic_compress_blocks(blocks, cblocks);

    free(blocks);

    unsigned char* buffer = ic_stream_compressed_blocks(cblocks, totalSize);

    ic_clean_up_compressed_blocks(cblocks);

    return buffer;
}

void ic_decode_image(const unsigned char* prevFrame, const unsigned char* data, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    compressed_macroblock_t* blocks = ic_unstream_compressed_blocks(data);

    for(compressed_macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        float data[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));
        float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

        rle_decode_block(data, block->rleData, block->rleSize);
        free(block->rleData);

        dct_decode_block(data, pixels);

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

    free(blocks);
}
