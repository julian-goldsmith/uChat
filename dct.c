#include <math.h>
#include <stdbool.h>
#include <xmmintrin.h>
#include "imgcoder.h"
#include "dct.h"

float dctPrecomp[MB_SIZE][MB_SIZE];
float quantPrecomp[MB_SIZE][MB_SIZE];

void dct_precompute_matrix()
{
    float log16_2 = logf(16.0f) * logf(16.0f);
    for (int u = 0; u < MB_SIZE; u++)
    {
        for (int x = 0; x < MB_SIZE; x++)
        {
            dctPrecomp[u][x] = cos((float)(2*x+1) * (float)u * M_PI / (2.0 * (float) MB_SIZE));//cos(M_PI * u * (x + 0.5) / MB_SIZE);

            quantPrecomp[u][x] = powf(16, -logf(u + 1) * logf(x + 1) / log16_2);
        }
    }
}

void _dct3_1d(__m128 in[MB_SIZE], __m128 out[MB_SIZE])
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

void _dct(float pixels[MB_SIZE][MB_SIZE][4], float data[MB_SIZE][MB_SIZE][4])
{
	__m128 in[MB_SIZE] __attribute__((aligned(16)));
	__m128 out[MB_SIZE] __attribute__((aligned(16)));
	__m128 rows[MB_SIZE][MB_SIZE] __attribute__((aligned(16)));

	// transform rows
	for (int j = 0; j<MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = _mm_load_ps(pixels[i][j]);
        }

		_dct3_1d(in, out);

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

		_dct3_1d(in, out);

		for (int i = 0; i < MB_SIZE; i++)
        {
            _mm_store_ps(data[i][j], out[i]);
        }
	}
}

void dct_quantize_block(float data[MB_SIZE][MB_SIZE][4], short qdata[MB_SIZE][MB_SIZE][3])
{
    const float quality = 16.0f;

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            data[x][y][0] *= quantPrecomp[x][y];
            data[x][y][1] *= quantPrecomp[x][y];
            data[x][y][2] *= quantPrecomp[x][y];

            if(fabs(data[x][y][0]) < quality)
            {
                data[x][y][0] = 0;
            }

            if(fabs(data[x][y][1]) < quality)
            {
                data[x][y][1] = 0;
            }

            if(fabs(data[x][y][2]) < quality)
            {
                data[x][y][2] = 0;
            }

            qdata[x][y][0] = (((short) data[x][y][0]) & 0xFFFE);
            qdata[x][y][1] = (((short) data[x][y][1]) & 0xFFFE);
            qdata[x][y][2] = (((short) data[x][y][2]) & 0xFFFE);
        }
    }
}

void dct_unquantize_block(short qdata[MB_SIZE][MB_SIZE][3], float data[MB_SIZE][MB_SIZE][4])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            data[x][y][0] = qdata[x][y][0] / quantPrecomp[x][y];
            data[x][y][1] = qdata[x][y][1] / quantPrecomp[x][y];
            data[x][y][2] = qdata[x][y][2] / quantPrecomp[x][y];
        }
    }
}

void dct_encode_block(unsigned char blockData[MB_SIZE][MB_SIZE][3], float blockDataDCT[MB_SIZE][MB_SIZE][4])
{
    float pixels[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            pixels[x][y][0] = blockData[x][y][0];
            pixels[x][y][1] = blockData[x][y][1];
            pixels[x][y][2] = blockData[x][y][2];
            pixels[x][y][3] = 0.0;
        }
    }

    _dct(pixels, blockDataDCT);
}

void _idct3_1d(__m128 in[MB_SIZE], __m128 out[MB_SIZE], bool clamp)
{
	for (int u = 0; u < MB_SIZE; u++)
	{
		__m128 z = _mm_set1_ps(0.0f);

		for (int x = 0; x < MB_SIZE; x++)
		{
		    __m128 co = _mm_set1_ps(1.0f);

            if (x == 0)
            {
                co = _mm_set1_ps(1.0f / sqrtf(2.0));
            }

            z = _mm_add_ps(z, _mm_mul_ps(co, _mm_mul_ps(_mm_set1_ps(0.5f), _mm_mul_ps(in[x], _mm_set1_ps(dctPrecomp[x][u])))));
		}

        if(clamp)
        {
            z = _mm_min_ps(_mm_max_ps(z, _mm_set1_ps(0.0f)), _mm_set1_ps(255.0f));  // clamp to max 255, min 0
        }

        out[u] = z;
	}
}

void idct(float pixels[MB_SIZE][MB_SIZE][4], float data[MB_SIZE][MB_SIZE][4])
{
	__m128 in[MB_SIZE] __attribute__((aligned(16)));
	__m128 out[MB_SIZE] __attribute__((aligned(16)));
	__m128 rows[MB_SIZE][MB_SIZE] __attribute__((aligned(16)));

	// transform rows
	for (int j = 0; j<MB_SIZE; j++)
	{
		for (int i = 0; i < MB_SIZE; i++)
        {
			in[i] = _mm_load_ps(data[i][j]);
        }

		_idct3_1d(in, out, false);

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

		_idct3_1d(in, out, true);

		for (int i = 0; i < MB_SIZE; i++)
        {
            _mm_store_ps(pixels[i][j], out[i]);
        }
	}
}

void dct_decode_block(float data[MB_SIZE][MB_SIZE][4], float pixels[MB_SIZE][MB_SIZE][4])
{
    idct(pixels, data);
}
