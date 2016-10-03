#include <math.h>
#include <stdbool.h>
#include <xmmintrin.h>
#include "imgcoder.h"
#include "dct.h"

float dct16Precomp[MB_SIZE][MB_SIZE];
float dct4Precomp[4][4];
float quantPrecomp[MB_SIZE][MB_SIZE];

void dct_precompute_matrix()
{
    float log16_2 = logf(16.0f) * logf(16.0f);
    for (int u = 0; u < MB_SIZE; u++)
    {
        for (int x = 0; x < MB_SIZE; x++)
        {
            dct16Precomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * (float) MB_SIZE));

            quantPrecomp[u][x] = powf(16, -logf(u + 1) * logf(x + 1) / log16_2);
        }
    }

    for(int u = 0; u < 4; u++)
    {
        for(int x = 0; x < 4; x++)
        {
            dct4Precomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * 4.0));
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

void dct_encode_block(unsigned char yin[MB_SIZE][MB_SIZE], unsigned char uin[MB_SIZE/4][MB_SIZE/4],
                      unsigned char vin[MB_SIZE/4][MB_SIZE/4], short yout[MB_SIZE][MB_SIZE],
                      short uout[MB_SIZE/4][MB_SIZE/4], short vout[MB_SIZE/4][MB_SIZE/4])
{
    dct16(yin, yout);
    dct4(uin, uout);
    dct4(vin, vout);
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

        out[u] = z;
	}
}

void idct16(short data[MB_SIZE][MB_SIZE], unsigned char pixels[MB_SIZE][MB_SIZE])
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

void idct4(short data[4][4], unsigned char pixels[4][4])
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

void dct_decode_block(short yin[MB_SIZE][MB_SIZE], short uin[4][4],
                      short vin[4][4], unsigned char yout[MB_SIZE][MB_SIZE],
                      unsigned char uout[4][4], unsigned char vout[4][4])
{
    idct16(yin, yout);
    idct4(uin, uout);
    idct4(vin, vout);
}
