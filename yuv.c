#include <xmmintrin.h>
#include <smmintrin.h>
#include "imgcoder.h"
#include "yuv.h"

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char yout[MB_SIZE][MB_SIZE],
                unsigned char uout[MB_SIZE/4][MB_SIZE/4], unsigned char vout[MB_SIZE/4][MB_SIZE/4])
{
    unsigned char tempuout[MB_SIZE][MB_SIZE];
    unsigned char tempvout[MB_SIZE][MB_SIZE];

    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            float temp[3];
            temp[0] = ( 0.299    * in[x][y][0]) + ( 0.587    * in[x][y][1]) + ( 0.114    * in[x][y][2]);
            temp[1] = (-0.168736 * in[x][y][0]) + (-0.331264 * in[x][y][1]) + ( 0.5      * in[x][y][2]) + 128;
            temp[2] = ( 0.5      * in[x][y][0]) + (-0.418688 * in[x][y][1]) + (-0.081312 * in[x][y][2]) + 128;

            yout[x][y] =
                (temp[0] > 255) ? 255 :
                (temp[0] < 0) ? 0 :
                temp[0];

            tempuout[x][y] =
                (temp[1] > 255) ? 255 :
                (temp[1] < 0) ? 0 :
                temp[1];

            tempvout[x][y] =
                (temp[2] > 255) ? 255 :
                (temp[2] < 0) ? 0 :
                temp[2];
        }
    }

    for(int x = 0; x < 4; x++)
    {
        for(int y = 0; y < 4; y++)
        {
            float blockU = 0;
            float blockV = 0;

            for(int blockx = 0; blockx < 4; blockx++)
            {
                for(int blocky = 0; blocky < 4; blocky++)
                {
                    blockU += tempuout[x * 4 + blockx][y * 4 + blocky];
                    blockV += tempvout[x * 4 + blockx][y * 4 + blocky];
                }
            }

            blockU /= 16;
            blockV /= 16;

            uout[x][y] =
                (blockU > 255) ? 255 :
                (blockU < 0) ? 0 :
                blockU;

            vout[x][y] =
                (blockV > 255) ? 255 :
                (blockV < 0) ? 0 :
                blockV;
        }
    }
}

void yuv_decode(const float yin[MB_SIZE][MB_SIZE][4], const unsigned char uin[MB_SIZE/4][MB_SIZE/4],
                const unsigned char vin[MB_SIZE/4][MB_SIZE/4], float out[MB_SIZE][MB_SIZE][4])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            float yp = yin[x][y][0];
            float u = uin[x/4][y/4];
            float v = vin[x/4][y/4];

            out[x][y][0] = yp                           + ( 1.402    * (v - 128));
            out[x][y][1] = yp + (-0.344136 * (u - 128)) + (-0.714136 * (v - 128));
            out[x][y][2] = yp + ( 1.772    * (u - 128));

            out[x][y][0] =
                (out[x][y][0] > 255) ? 255 :
                (out[x][y][0] < 0) ? 0 :
                out[x][y][0];

            out[x][y][1] =
                (out[x][y][1] > 255) ? 255 :
                (out[x][y][1] < 0) ? 0 :
                out[x][y][1];

            out[x][y][2] =
                (out[x][y][2] > 255) ? 255 :
                (out[x][y][2] < 0) ? 0 :
                out[x][y][2];
        }
    }
}
