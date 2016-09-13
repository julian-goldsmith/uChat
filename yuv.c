#include <xmmintrin.h>
#include <smmintrin.h>
#include "imgcoder.h"

__m128 m4x4v_rowSSE3(const __m128 rows[4], const __m128 v) {
  __m128 prod1 = _mm_mul_ps(rows[0], v);
  __m128 prod2 = _mm_mul_ps(rows[1], v);
  __m128 prod3 = _mm_mul_ps(rows[2], v);
  __m128 prod4 = _mm_mul_ps(rows[3], v);

  return _mm_hadd_ps(_mm_hadd_ps(prod1, prod2), _mm_hadd_ps(prod3, prod4));
}

void yuv_encode(char in[MB_SIZE][MB_SIZE][4], char out[MB_SIZE][MB_SIZE][4])
{
    float temp[MB_SIZE][MB_SIZE][4] __attribute__((aligned(16)));

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            temp[x][y][0] = in[x][y][0];
            temp[x][y][1] = in[x][y][1];
            temp[x][y][2] = in[x][y][2];
        }
    }

    __m128 mat[4];
    mat[0] = _mm_set_ps(0.2126, 0.7152, 0.0722, 0.0);
    mat[1] = _mm_set_ps(-0.09991, -0.33609, -0.436, 0.0);
    mat[2] = _mm_set_ps(0.615, -0.55861, -0.05639, 0.0);
    mat[3] = _mm_set_ps(0.0, 0.0, 0.0, 0.0);

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            __m128 pix = _mm_load_ps(temp[x][y]);
            _mm_store_ps(temp[x][y], m4x4v_rowSSE3(mat, pix));
        }
    }

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            out[x][y][0] = temp[x][y][0];
            out[x][y][1] = temp[x][y][1];
            out[x][y][2] = temp[x][y][2];
        }
    }
}

void yuv_decode(float in[MB_SIZE][MB_SIZE][4], float out[MB_SIZE][MB_SIZE][4])
{
    __m128 mat[4];
    mat[0] = _mm_set_ps(1, 0, 1.28033, 0.0);
    mat[1] = _mm_set_ps(1, -0.21482, -0.38059, 0.0);
    mat[2] = _mm_set_ps(1, 2.12798, 0.0, 0.0);
    mat[3] = _mm_set_ps(0.0, 0.0, 0.0, 0.0);

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            __m128 pix = _mm_load_ps(in[x][y]);
            _mm_store_ps(out[x][y], m4x4v_rowSSE3(mat, pix));
        }
    }
}
