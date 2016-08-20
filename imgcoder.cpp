#include <math.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
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
    double blockAvg[3];

    blockAvg[0] = 0.0;
    blockAvg[1] = 0.0;
    blockAvg[2] = 0.0;

    for(int blx = 0; blx < MB_SIZE; blx++)
    {
        for(int bly = 0; bly < MB_SIZE; bly++)
        {
            int frameBase = 3 * ((block->mb_y * MB_SIZE + bly) * 640 + (block->mb_x * MB_SIZE + blx));

            double dr = block->blockData[blx][bly][0] - prevFrame[frameBase];
            double dg = block->blockData[blx][bly][1] - prevFrame[frameBase + 1];
            double db = block->blockData[blx][bly][2] - prevFrame[frameBase + 2];

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
double dctPrecomp[MB_SIZE][MB_SIZE];

void precomputeDCTMatrix()
{
    for (int u = 0; u < MB_SIZE; u++)
    {
        for (int x = 0; x < MB_SIZE; x++)
        {
            dctPrecomp[u][x] = cos((double)(2*x+1) * (double)u * M_PI/32.0);//cos(M_PI * u * (x + 0.5) / MB_SIZE);
        }
    }
}

#define COEFFS(Cu,Cv,u,v) { \
	if (u == 0) Cu = 1.0 / sqrt(2.0); else Cu = 1.0; \
	if (v == 0) Cv = 1.0 / sqrt(2.0); else Cv = 1.0; \
	}
/*
void dct(unsigned char pixels[MB_SIZE][MB_SIZE], double data[16][16])
{
	int u,v,x,y;

	for (v=0; v < 16; v++)
	for (u=0; u < 16; u++)
	{
		double Cu, Cv, z = 0.0;

		COEFFS(Cu,Cv,u,v);

		for (y=0; y<16; y++)
		for (x=0; x<16; x++)
		{
			z += pixels[x][y] * dctPrecomp[u][x] * dctPrecomp[v][y];
		}

		data[v][u] = 0.125 * Cu * Cv * z;
	}
}*/

void dct_1d(double *in, double *out, const int count)
{
	int x, u;

	for (u=0; u < count; u++)
	{
		double z = 0;

		for (x=0; x < count; x++)
		{
			z += in[x] * dctPrecomp[u][x];
		}

		if (u == 0) z *= 1.0 / sqrt(2.0);
		out[u] = z / 4.0;
	}
}

void dct(unsigned char pixels[MB_SIZE][MB_SIZE], double data[16][16])
{
	int i,j;
	double in[16], out[16], rows[16][16];

	/* transform rows */
	for (j=0; j<16; j++)
	{
		for (i=0; i<16; i++)
			in[i] = pixels[i][j];
		dct_1d(in, out, 16);
		for (i=0; i<16; i++) rows[j][i] = out[i];
	}

	/* transform columns */
	for (j=0; j<16; j++)
	{
		for (i=0; i<16; i++)
			in[i] = rows[i][j];
		dct_1d(in, out, 16);
		for (i=0; i<16; i++) data[i][j] = out[i];
	}
}

void dctQuantize(double data[MB_SIZE][MB_SIZE])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            if(fabs(data[x][y]) < 10.0)
            {
                data[x][y] = 0;
            }
        }
    }
}

void idct(unsigned char pixels[MB_SIZE][MB_SIZE], double data[16][16])
{
	int u,v,x,y;

	/* iDCT */
	for (y=0; y < 16; y++)
	for (x=0; x < 16; x++)
	{
		double z = 0.0;

		for (v=0; v < 16; v++)
		for (u=0; u < 16; u++)
		{
			double Cu, Cv;

			COEFFS(Cu,Cv,u,v);

			z += Cu * Cv * data[v][u] * dctPrecomp[u][x] * dctPrecomp[v][y];
		}

		z /= 4.0;
		if (z > 255.0) z = 255.0;
		if (z < 0) z = 0.0;

		pixels[x][y] = (unsigned char) z;
	}
}

void dctBlock(macroblock_t* block)
{
    unsigned char pixels[3][MB_SIZE][MB_SIZE];

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            pixels[0][x][y] = block->blockData[x][y][0];
            pixels[1][x][y] = block->blockData[x][y][1];
            pixels[2][x][y] = block->blockData[x][y][2];
        }
    }

    dct(pixels[0], block->blockDataDCT[0]);
    dct(pixels[1], block->blockDataDCT[1]);
    dct(pixels[2], block->blockDataDCT[2]);

    dctQuantize(block->blockDataDCT[0]);
    dctQuantize(block->blockDataDCT[1]);
    dctQuantize(block->blockDataDCT[2]);
}

double* rleValue(double data[MB_SIZE][MB_SIZE])
{
    // loop through the data.  if the current value is the same as the last, increment counter.  if it differs, write counter and value, then reset counter
    double* odata = (double*) calloc(MB_SIZE * MB_SIZE * 2, sizeof(double));    // resize later
    int odatacount = 0;

    double counter = 0.0;
    double value = data[0][0];

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            if(value == data[x][y])
            {
                counter++;
            }
            else
            {
                odata[odatacount++] = counter;
                odata[odatacount++] = value;
                counter = 0.0;
            }
        }
    }

    return odata;
}

void unrleValue(double data[MB_SIZE][MB_SIZE], double* rleData)
{
    double counter = 0.0;
    int x = 0;
    int y = 0;

    for(int i = 0; i < MB_SIZE * MB_SIZE; i++)
    {
        counter = rleData[i * 2];

        while(counter > 0.0)
        {
            counter--;

            if(x == MB_SIZE)
            {
                x = 0;
                y++;
            }

            data[x][y] = rleData[i * 2 + 1];
        }
    }
}

double* rleBlock(macroblock_t* block)
{
    block->rleData = rleValue(block->blockDataDCT[0]);
}

void unrleBlock(macroblock_t* block)
{
    unrleValue(block->blockDataDCT[0], block->rleData);
}

void idctBlock(macroblock_t* block)
{
    unsigned char pixels[3][MB_SIZE][MB_SIZE];

    idct(pixels[0], block->blockDataDCT[0]);
    idct(pixels[1], block->blockDataDCT[1]);
    idct(pixels[2], block->blockDataDCT[2]);

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            block->blockData[x][y][0] = pixels[0][x][y];
            block->blockData[x][y][1] = pixels[1][x][y];
            block->blockData[x][y][2] = pixels[2][x][y];
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

void dctBlocks(macroblock_t *blocks)
{
    for(macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        dctBlock(block);
        rleBlock(block);
    }
}

void showRMS(const macroblock_t* blocks, unsigned char* rmsView)
{
    double rmsMax = 0.0;
    memset(rmsView, 0, 640*480*3);

    for(const macroblock_t* block = blocks; block < blocks + (MB_NUM_X * MB_NUM_Y); block++)
    {
        if(block->rms > rmsMax)
            rmsMax = block->rms;
    }

    for(const macroblock_t* block = blocks; block < blocks + (MB_NUM_X * MB_NUM_Y); block++)
    {
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);
                rmsView[frameBase] = 256 * (block->rms / rmsMax);
                rmsView[frameBase + 1] = 256 * (block->rms / rmsMax);
                rmsView[frameBase + 2] = 256 * (block->rms / rmsMax);
            }
        }
    }
}

void encodeImage(const unsigned char* imgIn, const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* rmsView)
{
    fillInBlocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), sortblocks);
    dctBlocks(blocks);

    showRMS(blocks, rmsView);
}

void decodeImage(const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* frameOut)
{
    memcpy(frameOut, prevFrame, 3 * 640 * 480);

    for(macroblock_t* block = blocks; block < blocks + NUMBLOCKS; block++)
    {
        // clear for testing
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                block->blockData[x][y][0] = 0;
                block->blockData[x][y][1] = 0;
                block->blockData[x][y][2] = 0;
            }
        }

        unrleBlock(block);
        idctBlock(block);

        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                int frameBase = 3 * ((block->mb_y * MB_SIZE + y) * 640 + block->mb_x * MB_SIZE + x);
                frameOut[frameBase] = block->blockData[x][y][0];
                frameOut[frameBase + 1] = block->blockData[x][y][1];
                frameOut[frameBase + 2] = block->blockData[x][y][2];
            }
        }
    }
}
