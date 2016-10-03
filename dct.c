#include <math.h>
#include <stdbool.h>
#include <xmmintrin.h>
#include "imgcoder.h"
#include "dct.h"

float dct16Precomp[MB_SIZE][MB_SIZE];
float dct4Precomp[4][4];
float quant16Precomp[MB_SIZE][MB_SIZE];
float quant4Precomp[4][4];

void dct_precompute_matrix()
{
    float log16_2 = logf(16.0f) * logf(16.0f);
    float log4_2 = logf(4.0f) * logf(4.0f);

    for (int u = 0; u < MB_SIZE; u++)
    {
        for (int x = 0; x < MB_SIZE; x++)
        {
            dct16Precomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * (float) MB_SIZE));

            quant16Precomp[u][x] = powf(16, -logf(u + 1) * logf(x + 1) / log16_2);
        }
    }

    for(int u = 0; u < 4; u++)
    {
        for(int x = 0; x < 4; x++)
        {
            dct4Precomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * 4.0));

            quant4Precomp[u][x] = powf(4, -logf(u + 1) * logf(x + 1) / log4_2);
        }
    }
}

void dct16_1d(float in[MB_SIZE], float out[MB_SIZE])
{
	for (int u = 0; u < MB_SIZE; u++)
	{
		float z = 0.0f;

		for (int x = 0; x < MB_SIZE; x++)
		{
            z += in[x] * dct16Precomp[u][x];
		}

		if (u == 0)
        {
            z *= 1.0f / sqrtf(2.0);
        }

        out[u] = z / 4.0f;
	}
}

void dct4_1d(float in[MB_SIZE], float out[MB_SIZE])
{
	for (int u = 0; u < 4; u++)
	{
		float z = 0.0f;

		for (int x = 0; x < 4; x++)
		{
            z += in[x] * dct4Precomp[u][x];
		}

		if (u == 0)
        {
            z *= 1.0f / sqrtf(2.0);
        }

        out[u] = z / 2.0f;      // FIXME: should this be 2.0f for this size?
	}
}

void dct16(unsigned char pixels[MB_SIZE][MB_SIZE], short data[MB_SIZE][MB_SIZE])
{
	float in[MB_SIZE];
	float out[MB_SIZE];
	float rows[MB_SIZE][MB_SIZE];

	// transform rows
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = pixels[i][j];
        }

		dct16_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
		{
            rows[j][i] = out[i];
		}
	}

	// transform columns
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = rows[i][j];
        }

		dct16_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
        {
            data[i][j] = out[i];
        }
	}
}

void dct4(unsigned char pixels[4][4], short data[4][4])
{
	float in[4];
	float out[4];
	float rows[4][4];

	// transform rows
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
        {
			in[i] = pixels[i][j];
        }

		dct4_1d(in, out);

		for (int i = 0; i < 4; i++)
		{
            rows[j][i] = out[i];
		}
	}

	// transform columns
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
        {
			in[i] = rows[i][j];
        }

		dct4_1d(in, out);

		for (int i = 0; i < 4; i++)
        {
            data[i][j] = out[i];
        }
	}
}

void dct16_quantize_block(short out[MB_SIZE][MB_SIZE])
{
    const float quality = 8.0f;

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            float val = out[x][y] * quant16Precomp[x][y];

            if(fabs(val) < quality)
            {
                val = 0;
            }

            out[x][y] = (short) val;
        }
    }
}

void dct4_quantize_block(short out[4][4])
{
    const float quality = 8.0f;

    for(int x = 0; x < 4; x++)
    {
        for(int y = 0; y < 4; y++)
        {
            float val = out[x][y] * quant4Precomp[x][y];

            if(fabs(val) < quality)
            {
                val = 0;
            }

            out[x][y] = (short) val;
        }
    }
}

void dct_encode_block(unsigned char yin[MB_SIZE][MB_SIZE], unsigned char uin[MB_SIZE/4][MB_SIZE/4],
                      unsigned char vin[MB_SIZE/4][MB_SIZE/4], short yout[MB_SIZE][MB_SIZE],
                      short uout[MB_SIZE/4][MB_SIZE/4], short vout[MB_SIZE/4][MB_SIZE/4])
{
    dct16(yin, yout);
    dct16_quantize_block(yout);

    dct4(uin, uout);
    dct4_quantize_block(uout);

    dct4(vin, vout);
    dct4_quantize_block(vout);
}

void idct16_1d(float in[MB_SIZE], float out[MB_SIZE], bool clamp)
{
	for (int u = 0; u < MB_SIZE; u++)
	{
		float z = 0.0f;

		for (int x = 0; x < MB_SIZE; x++)
		{
		    float co = 1.0f;

            if (x == 0)
            {
                co = 1.0f / sqrtf(2.0);
            }

            z += co * 0.5 * in[x] * dct16Precomp[x][u];
		}

        if(clamp)
        {
            z = (z > 255.0f) ? 255.0f :
                (z < 0.0f) ? 0.0f :
                z;
        }

        out[u] = z;
	}
}

void idct4_1d(float in[4], float out[4], bool clamp)
{
	for (int u = 0; u < 4; u++)
	{
		float z = 0.0f;

		for (int x = 0; x < 4; x++)
		{
		    float co = 1.0f;

            if (x == 0)
            {
                co = 1.0f / sqrtf(2.0);
            }

            z += co * 0.25 * in[x] * dct4Precomp[x][u];
		}

        if(clamp)
        {
            z = (z > 255.0f) ? 255.0f :
                (z < 0.0f) ? 0.0f :
                z;
        }

        out[u] = z * 4.0;
	}
}

void idct16(const short data[MB_SIZE][MB_SIZE], unsigned char pixels[MB_SIZE][MB_SIZE])
{
	float in[MB_SIZE];
	float out[MB_SIZE];
	float rows[MB_SIZE][MB_SIZE];

	// transform rows
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = data[i][j];
        }

		idct16_1d(in, out, false);

		for (int i = 0; i < MB_SIZE; i++)
		{
            rows[j][i] = out[i];
		}
	}

	// transform columns
	for (int j = 0; j < MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = rows[i][j];
        }

		idct16_1d(in, out, true);

		for (int i = 0; i < MB_SIZE; i++)
        {
            pixels[i][j] = out[i];
        }
	}
}

void idct4(const short data[4][4], unsigned char pixels[4][4])
{
	float in[4];
	float out[4];
	float rows[4][4];

	// transform rows
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
        {
			in[i] = data[i][j];
        }

		idct4_1d(in, out, false);

		for (int i = 0; i < 4; i++)
		{
            rows[j][i] = out[i];
		}
	}

	// transform columns
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
        {
			in[i] = rows[i][j];
        }

		idct4_1d(in, out, true);

		for (int i = 0; i < 4; i++)
        {
            pixels[i][j] = out[i];
        }
	}
}

void dct16_unquantize_block(const short yin[MB_SIZE][MB_SIZE], short yout[MB_SIZE][MB_SIZE])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            yout[x][y] = yin[x][y] / quant16Precomp[x][y];
        }
    }
}

void dct4_unquantize_block(const short in[4][4], short out[4][4])
{
    for(int x = 0; x < 4; x++)
    {
        for(int y = 0; y < 4; y++)
        {
            out[x][y] = in[x][y] / quant4Precomp[x][y];
        }
    }
}

void dct_decode_block(const short yin[MB_SIZE][MB_SIZE], const short uin[4][4],
                      const short vin[4][4], unsigned char yout[MB_SIZE][MB_SIZE],
                      unsigned char uout[4][4], unsigned char vout[4][4])
{
    short ytemp[MB_SIZE][MB_SIZE];
    dct16_unquantize_block(yin, ytemp);
    idct16((const short(*)[MB_SIZE]) ytemp, yout);

    short utemp[4][4];
    dct4_unquantize_block(uin, utemp);
    idct4((const short(*)[4]) utemp, uout);

    short vtemp[4][4];
    dct4_unquantize_block(vin, vtemp);
    idct4((const short(*)[4]) vtemp, vout);
}
