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
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);

                if(bnum < numBlocks)
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

void ic_compress_blocks(macroblock_t* blocks, short numBlocks, compressed_macroblock_t* cblocks)
{
    float blockDataDCT[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));  // Red Green Blue Unused

    for(int i = 0; i < numBlocks; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;

        dct_encode_block(block->blockData, blockDataDCT);
        dct_quantize_block(blockDataDCT, cblock->blockDataQuant);
    }
}

unsigned char* ic_stream_compressed_blocks(const compressed_macroblock_t* cblocks, int* totalSize, short numBlocks)
{
    int size = sizeof(short) + numBlocks * sizeof(compressed_macroblock_t);

    unsigned char* buffer = (unsigned char*) malloc(size);
    unsigned char* bp = buffer;

    *(short*) bp = numBlocks;
    bp += sizeof(short);

    for(const compressed_macroblock_t* cblock = cblocks; cblock < cblocks + numBlocks; cblock++)
    {
        memcpy(bp, cblock, sizeof(compressed_macroblock_t));
        bp += sizeof(compressed_macroblock_t);
    }

    unsigned char* compressed = huffman_encode(buffer, size, totalSize);

    free(buffer);

    return compressed;
}

compressed_macroblock_t* ic_unstream_compressed_blocks(const unsigned char* data, const int datalen, short* numBlocks)
{
    int outdatalen;
    unsigned char* uncompressed = huffman_decode(data, datalen, &outdatalen);

    *numBlocks = *(short*) uncompressed;

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) malloc(sizeof(compressed_macroblock_t) * *numBlocks);

    const unsigned char* bp = uncompressed + sizeof(short);

    for(compressed_macroblock_t* cblock = cblocks; cblock < cblocks + *numBlocks; cblock++)
    {
        memcpy(cblock, bp, sizeof(compressed_macroblock_t));
        bp += sizeof(compressed_macroblock_t);
    }

    free(uncompressed);

    return cblocks;
}

void ic_clean_up_compressed_blocks(compressed_macroblock_t* cblocks, short numBlocks)
{
    free(cblocks);
}

short ic_get_num_blocks(const macroblock_t* blocks)
{
    const float rmsMin = 16.0;

    for(short i = 0; i < (MB_NUM_X * MB_NUM_Y) / 16; i++)
    {
        if(blocks[i].rms < rmsMin)
            return i;
    }

    return (MB_NUM_X * MB_NUM_Y) / 16;      // FIXME: make const
}

unsigned char* ic_encode_image(const unsigned char* imgIn, const unsigned char* prevFrame, unsigned char* rmsView, int* totalSize)
{
    macroblock_t* blocks = (macroblock_t*) malloc(MB_NUM_X * MB_NUM_Y * sizeof(macroblock_t));

    ic_fill_blocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), ic_sort_blocks);

    short numBlocks = ic_get_num_blocks(blocks);
    ic_show_rms(blocks, rmsView, numBlocks);

    compressed_macroblock_t* cblocks = (compressed_macroblock_t*) calloc(numBlocks, sizeof(compressed_macroblock_t));
    ic_compress_blocks(blocks, numBlocks, cblocks);

    free(blocks);

    unsigned char* buffer = ic_stream_compressed_blocks(cblocks, totalSize, numBlocks);

    ic_clean_up_compressed_blocks(cblocks, numBlocks);

    return buffer;
}

void ic_decode_image(const unsigned char* prevFrame, const unsigned char* data, const int datalen, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    short numBlocks;
    compressed_macroblock_t* blocks = ic_unstream_compressed_blocks(data, datalen, &numBlocks);

    for(compressed_macroblock_t* block = blocks; block < blocks + numBlocks; block++)
    {
        float data[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));
        float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                data[x][y][0] = block->blockDataQuant[x][y][0];
                data[x][y][1] = block->blockDataQuant[x][y][1];
                data[x][y][2] = block->blockDataQuant[x][y][2];
            }
        }

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
