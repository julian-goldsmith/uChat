#include <math.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <xmmintrin.h>
#include "imgcoder.h"

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

// [u][x]
float dctPrecomp[MB_SIZE][MB_SIZE];

void precomputeDCTMatrix()
{
    for (int u = 0; u < MB_SIZE; u++)
    {
        for (int x = 0; x < MB_SIZE; x++)
        {
            dctPrecomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * (float) MB_SIZE));//cos(M_PI * u * (x + 0.5) / MB_SIZE);
        }
    }
}

float idctPrecomp[MB_SIZE][MB_SIZE][MB_SIZE][MB_SIZE];

void dct3_1d(__m128 in[MB_SIZE], __m128 out[MB_SIZE])
{
	for (int u = 0; u < MB_SIZE; u++)
	{
		__m128 z = _mm_set1_ps(0.0f);

		for (int x = 0; x < MB_SIZE; x++)
		{
            z = _mm_add_ps(z, _mm_mul_ps(in[x], _mm_set1_ps(dctPrecomp[u][x])));
		}

		if (u == 0)
        {
            z = _mm_mul_ps(z, _mm_set1_ps(1.0f / sqrtf(2.0)));
        }

        out[u] = _mm_mul_ps(z, _mm_set1_ps(0.25f));
	}
}

void dct(float pixels[MB_SIZE][MB_SIZE][4], float data[MB_SIZE][MB_SIZE][4])
{
	__m128 in[MB_SIZE] __attribute__((aligned(16)));
	__m128 out[MB_SIZE] __attribute__((aligned(16)));
	__m128 rows[MB_SIZE][MB_SIZE] __attribute__((aligned(16)));

	/* transform rows */
	for (int j = 0; j<MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = _mm_load_ps(pixels[i][j]);
        }

		dct3_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
		{
            rows[j][i] = out[i];
		}
	}

	/* transform columns */
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = rows[i][j];
        }

		dct3_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
        {
            _mm_store_ps(data[i][j], out[i]);
        }
	}
}

void dctQuantize(float data[MB_SIZE][MB_SIZE][4])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            if(fabs(data[x][y][0]) < 10.0)
            {
                data[x][y][0] = 0;
            }

            if(fabs(data[x][y][1]) < 10.0)
            {
                data[x][y][1] = 0;
            }

            if(fabs(data[x][y][2]) < 10.0)
            {
                data[x][y][2] = 0;
            }

            // this one is unnecessary
            if(fabs(data[x][y][3]) < 10.0)
            {
                data[x][y][3] = 0;
            }
        }
    }
}

void dctBlock(macroblock_t* block)
{
    float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            pixels[x][y][0] = block->blockData[x][y][0];
            pixels[x][y][1] = block->blockData[x][y][1];
            pixels[x][y][2] = block->blockData[x][y][2];
            pixels[x][y][3] = 0.0;
        }
    }

    dct(pixels, block->blockDataDCT);
    dctQuantize(block->blockDataDCT);
}

void idct(float pixels[MB_SIZE][MB_SIZE][4], float data[MB_SIZE][MB_SIZE][4])
{
	for (int y = 0; y < MB_SIZE; y++)
	for (int x = 0; x < MB_SIZE; x++)
	{
		__m128 z = _mm_set1_ps(0.0f);

		for (int v=0; v < MB_SIZE; v++)
		for (int u=0; u < MB_SIZE; u++)
		{
			__m128 Cu = _mm_set1_ps((u == 0) ? 0.353553390f : 0.5f);        // 0.5f / sqrtf(2.0f)
			__m128 Cv = _mm_set1_ps((v == 0) ? 0.353553390f : 0.5f);

            // z += dctPrecomp[v][y] * dctPrecomp[u][x] * data[v][u] * Cu * Cv
			z = _mm_add_ps(z,
                _mm_mul_ps(_mm_set1_ps(idctPrecomp[v][y][u][x]),
                    _mm_mul_ps(_mm_load_ps(data[v][u]), _mm_mul_ps(Cu, Cv))));
		}

		z = _mm_min_ps(_mm_max_ps(z, _mm_set1_ps(0.0f)), _mm_set1_ps(255.0f));  // clamp to max 255, min 0

		_mm_storeu_ps(pixels[x][y], z);
	}
}

void precomputeIDCTMatrix()
{
    for (int y = 0; y < MB_SIZE; y++)
	for (int x = 0; x < MB_SIZE; x++)
	{
		for (int v=0; v < MB_SIZE; v++)
		for (int u=0; u < MB_SIZE; u++)
		{
		    idctPrecomp[v][y][u][x] = dctPrecomp[v][y] * dctPrecomp[u][x];
		}
	}
}

void idctBlock(float data[MB_SIZE][MB_SIZE][4], float pixels[MB_SIZE][MB_SIZE][4])
{
    idct(pixels, data);
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

void dctBlocks(macroblock_t *blocks)
{
    for(macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        dctBlock(block);
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

void rleBlock(macroblock_t* block)
{
    block->rleData = rleValue(block->blockDataDCT, &block->rleSize);
}

void rleBlocks(macroblock_t *blocks)
{
    for(macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        rleBlock(block);
    }
}

void unrleBlock(compressed_macroblock_t* block, float data[MB_SIZE][MB_SIZE][4])
{
    unrleValue(data, block->rleData, block->rleSize);

    free(block->rleData);
}

void convertBlocksToCBlocks(macroblock_t* blocks, compressed_macroblock_t* cblocks)
{
    for(int i = 0; i < NUMBLOCKS; i++)
    {
        macroblock_t* block = blocks + i;
        compressed_macroblock_t* cblock = cblocks + i;

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;
        cblock->rleData = block->rleData;
        cblock->rleSize = block->rleSize;
    }
}

compressed_macroblock_t* encodeImage(const unsigned char* imgIn, const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* rmsView)
{
    fillInBlocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), sortblocks);
    dctBlocks(blocks);
    rleBlocks(blocks);

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
        float data[MB_SIZE][MB_SIZE][4];
        float pixels[MB_SIZE][MB_SIZE][4];

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
        idctBlock(data, pixels);

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
