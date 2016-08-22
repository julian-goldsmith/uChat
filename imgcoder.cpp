#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "imgcoder.h"
#include "dct.h"

int sortblocks(const void* val1, const void* val2)
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

void calculateBlockRMS(macroblock_t* block, const unsigned char* prevFrame)
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

void buildBlock(macroblock_t* block, const unsigned char* imgIn)
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

void fillInBlocks(macroblock_t* blocks, const unsigned char* imgIn, const unsigned char* prevFrame)
{
    for(int x = 0; x < MB_NUM_X; x++)
    {
        for(int y = 0; y < MB_NUM_Y; y++)
        {
            macroblock_t* block = blocks + y * MB_NUM_X + x;
            block->mb_x = x;
            block->mb_y = y;
            block->rms = 0.0;
            buildBlock(block, imgIn);

            calculateBlockRMS(block, prevFrame);
        }
    }
}

void showRMS(const macroblock_t* blocks, unsigned char* rmsView)
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

float* rleValue(float data[MB_SIZE][MB_SIZE][4], int* rleSize)
{
    float* odata = (float*) calloc(MB_SIZE * MB_SIZE * 5, sizeof(float));    // resize later
    int odatacount = 0;

    float counter = 0.0;

    float value[3];
    value[0] = data[0][0][0];
    value[1] = data[0][0][1];
    value[2] = data[0][0][2];

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            if(value[0] == data[x][y][0] &&
               value[1] == data[x][y][1] &&
               value[2] == data[x][y][2])
            {
                counter++;
            }
            else
            {
                odata[odatacount++] = counter;
                odata[odatacount++] = value[0];
                odata[odatacount++] = value[1];
                odata[odatacount++] = value[2];

                counter = 1.0;
                value[0] = data[x][y][0];
                value[1] = data[x][y][1];
                value[2] = data[x][y][2];
            }
        }
    }

    odata[odatacount++] = counter;
    odata[odatacount++] = value[0];
    odata[odatacount++] = value[1];
    odata[odatacount++] = value[2];

    *rleSize = odatacount;

    return (float*) realloc(odata, sizeof(float) * odatacount);
}

void unrleValue(float data[MB_SIZE][MB_SIZE][4], float* rleData, int rleSize)
{
    float counter = 0.0;
    int x = 0;
    int y = 0;

    for(int i = 0; i < rleSize; i += 4)
    {
        counter = rleData[i];

        while(counter > 0.0)
        {
            data[y][x][0] = rleData[i + 1];
            data[y][x][1] = rleData[i + 2];
            data[y][x][2] = rleData[i + 3];
            data[y][x][3] = 0.0f;

            counter--;
            x++;

            if(x == MB_SIZE)
            {
                x = 0;
                y++;
            }
        }
    }
}

void unrleBlock(compressed_macroblock_t* block, float data[MB_SIZE][MB_SIZE][4])
{
    unrleValue(data, block->rleData, block->rleSize);

    free(block->rleData);
}

void convertBlocksToCBlocks(macroblock_t* blocks, compressed_macroblock_t* cblocks)
{
    float blockDataDCT[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));  // Red Green Blue Unused

    for(int i = 0; i < NUMBLOCKS; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        dct_encode_block(block->blockData, blockDataDCT);
        dct_quantize_block(blockDataDCT);

        int rleSize;
        float* rleData = rleValue(blockDataDCT, &rleSize);

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;
        cblock->rleData = rleData;
        cblock->rleSize = rleSize;
    }
}

compressed_macroblock_t* encodeImage(const unsigned char* imgIn, const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* rmsView)
{
    fillInBlocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), sortblocks);

    showRMS(blocks, rmsView);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) calloc(NUMBLOCKS, sizeof(compressed_macroblock_t));
    convertBlocksToCBlocks(blocks, cblocks);
    return cblocks;
}

void decodeImage(const unsigned char* prevFrame, compressed_macroblock_t* blocks, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    for(compressed_macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        float data[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));
        float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

        // clear data, so missing values are 0
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                data[x][y][0] = 0;
                data[x][y][1] = 0;
                data[x][y][2] = 0;
                data[x][y][3] = 0;

                pixels[x][y][0] = 0;
                pixels[x][y][1] = 0;
                pixels[x][y][2] = 0;
                pixels[x][y][3] = 0;
            }
        }

        unrleBlock(block, data);
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
}
