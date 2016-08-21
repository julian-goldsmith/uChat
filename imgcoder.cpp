#include <math.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
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

void dct3_1d(double in[MB_SIZE][4], double out[MB_SIZE][4])
{
	for (int u = 0; u < MB_SIZE; u++)
	{
		double z[4];

		z[0] = 0.0;
		z[1] = 0.0;
		z[2] = 0.0;
		z[3] = 0.0;

		for (int x = 0; x < MB_SIZE; x++)
		{
			z[0] += in[x][0] * dctPrecomp[u][x];
			z[1] += in[x][0] * dctPrecomp[u][x];
			z[2] += in[x][0] * dctPrecomp[u][x];
			z[3] += in[x][0] * dctPrecomp[u][x];
		}

		if (u == 0)
        {
            z[0] *= 1.0 / sqrt(2.0);
            z[1] *= 1.0 / sqrt(2.0);
            z[2] *= 1.0 / sqrt(2.0);
            z[3] *= 1.0 / sqrt(2.0);
        }

		out[u][0] = z[0] / 4.0;
		out[u][1] = z[1] / 4.0;
		out[u][2] = z[2] / 4.0;
		out[u][3] = z[3] / 4.0;
	}
}

void dct(double pixels[MB_SIZE][MB_SIZE][4], double data[MB_SIZE][MB_SIZE][4])
{
	double in[MB_SIZE][4] __attribute__((aligned(16)));
	double out[MB_SIZE][4]  __attribute__((aligned(16)));
	double rows[MB_SIZE][MB_SIZE][4]  __attribute__((aligned(16)));

	/* transform rows */
	for (int j = 0; j<MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i][0] = pixels[i][j][0];
			in[i][1] = pixels[i][j][1];
			in[i][2] = pixels[i][j][2];
			in[i][3] = pixels[i][j][3];
        }

		dct3_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
		{
            rows[j][i][0] = out[i][0];
            rows[j][i][1] = out[i][1];
            rows[j][i][2] = out[i][2];
            rows[j][i][3] = out[i][3];
		}
	}

	/* transform columns */
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i][0] = rows[i][j][0];
			in[i][1] = rows[i][j][1];
			in[i][2] = rows[i][j][2];
			in[i][3] = rows[i][j][3];
        }

		dct3_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
        {
            data[i][j][0] = out[i][0];
            data[i][j][1] = out[i][1];
            data[i][j][2] = out[i][2];
            data[i][j][3] = out[i][3];
        }
	}
}

void dctQuantize(double data[MB_SIZE][MB_SIZE][4])
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

#define COEFFS(Cu,Cv,u,v) { \
	if (u == 0) Cu = 1.0 / sqrt(2.0); else Cu = 1.0; \
	if (v == 0) Cv = 1.0 / sqrt(2.0); else Cv = 1.0; \
	}

void idct(double pixels[MB_SIZE][MB_SIZE][3], double data[MB_SIZE][MB_SIZE][4])
{
    // FIXME: convert to 1d, like the other one
	int u,v,x,y;

	/* iDCT */
	for (y=0; y < MB_SIZE; y++)
	for (x=0; x < MB_SIZE; x++)
	{
		double z[4];

		z[0] = 0.0;
		z[1] = 0.0;
		z[2] = 0.0;
		z[3] = 0.0;

		for (v=0; v < MB_SIZE; v++)
		for (u=0; u < MB_SIZE; u++)
		{
			double Cu, Cv;

			COEFFS(Cu,Cv,u,v);

			z[0] += Cu * Cv * data[v][u][0] * dctPrecomp[u][x] * dctPrecomp[v][y];
			z[1] += Cu * Cv * data[v][u][1] * dctPrecomp[u][x] * dctPrecomp[v][y];
			z[2] += Cu * Cv * data[v][u][2] * dctPrecomp[u][x] * dctPrecomp[v][y];
			z[3] += Cu * Cv * data[v][u][3] * dctPrecomp[u][x] * dctPrecomp[v][y];
		}

		z[0] /= 4.0;
		z[1] /= 4.0;
		z[2] /= 4.0;
		z[3] /= 4.0;

		if (z[0] > 255.0) z[0] = 255.0;
		if (z[1] > 255.0) z[1] = 255.0;
		if (z[2] > 255.0) z[2] = 255.0;
		if (z[3] > 255.0) z[3] = 255.0;

		if (z[0] < 0) z[0] = 0.0;
		if (z[1] < 0) z[1] = 0.0;
		if (z[2] < 0) z[2] = 0.0;
		if (z[3] < 0) z[3] = 0.0;

		pixels[x][y][0] = z[0];
		pixels[x][y][1] = z[1];
		pixels[x][y][2] = z[2];
		//pixels[x][y][3] = z[3];
	}
}

void dctBlock(macroblock_t* block)
{
    double pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

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
/*
double* rleValue(double data[MB_SIZE][MB_SIZE], int* rleSize)
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

                counter = 1.0;
                value = data[x][y];
            }
        }
    }

    *rleSize = odatacount;

    return (double*) realloc(odata, sizeof(double) * odatacount);
}

void unrleValue(double data[MB_SIZE][MB_SIZE], double* rleData, int rleSize)
{
    double counter = 0.0;
    int x = 0;
    int y = 0;

    for(int i = 0; i < rleSize; i += 2)
    {
        counter = rleData[i];

        while(counter > 0.0)
        {
            data[y][x] = rleData[i + 1];

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
    block->rleData[0] = rleValue(block->blockDataDCT[0], &block->rleSize[0]);
    block->rleData[1] = rleValue(block->blockDataDCT[1], &block->rleSize[1]);
    block->rleData[2] = rleValue(block->blockDataDCT[2], &block->rleSize[2]);
}

void unrleBlock(compressed_macroblock_t* block, double data[3][MB_SIZE][MB_SIZE])
{
    unrleValue(data[0], block->rleData[0], block->rleSize[0]);
    unrleValue(data[1], block->rleData[1], block->rleSize[1]);
    unrleValue(data[2], block->rleData[2], block->rleSize[2]);

    free(block->rleData[0]);
    free(block->rleData[1]);
    free(block->rleData[2]);
}
*/

void idctBlock(double data[MB_SIZE][MB_SIZE][4], double pixels[MB_SIZE][MB_SIZE][3])
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
        //rleBlock(block);
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

void convertBlocksToCBlocks(macroblock_t* blocks, compressed_macroblock_t* cblocks)
{
    for(int i = 0; i < NUMBLOCKS; i++)
    {
        macroblock_t* block = &blocks[i];
        compressed_macroblock_t* cblock = &cblocks[i];

        cblock->mb_x = block->mb_x;
        cblock->mb_y = block->mb_y;
        /*cblock->rleData[0] = block->rleData[0];
        cblock->rleData[1] = block->rleData[1];
        cblock->rleData[2] = block->rleData[2];
        cblock->rleSize[0] = block->rleSize[0];
        cblock->rleSize[1] = block->rleSize[1];
        cblock->rleSize[2] = block->rleSize[2];*/

        for(int x = 0; x < MB_SIZE; x++)
        for(int y = 0; y < MB_SIZE; y++)
        {
            cblock->blockDataDCT[x][y][0] = block->blockDataDCT[x][y][0];
            cblock->blockDataDCT[x][y][1] = block->blockDataDCT[x][y][1];
            cblock->blockDataDCT[x][y][2] = block->blockDataDCT[x][y][2];
            cblock->blockDataDCT[x][y][3] = block->blockDataDCT[x][y][3];
        }
    }
}

compressed_macroblock_t* encodeImage(const unsigned char* imgIn, const unsigned char* prevFrame, macroblock_t* blocks, unsigned char* rmsView)
{
    fillInBlocks(blocks, imgIn, prevFrame);
    qsort(blocks, MB_NUM_X * MB_NUM_Y, sizeof(macroblock_t), sortblocks);
    dctBlocks(blocks);

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
        double data[3][MB_SIZE][MB_SIZE];
        double pixels[MB_SIZE][MB_SIZE][3];

        // clear data, so missing values are 0
        for(int x = 0; x < MB_SIZE; x++)
        {
            for(int y = 0; y < MB_SIZE; y++)
            {
                data[0][x][y] = 0;
                data[1][x][y] = 0;
                data[2][x][y] = 0;
            }
        }

        //unrleBlock(block, data);
        //idctBlock(data, pixels);
        idctBlock(block->blockDataDCT, pixels);

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
