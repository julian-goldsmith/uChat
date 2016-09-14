#include <xmmintrin.h>
#include <smmintrin.h>
#include "imgcoder.h"

void yuv_encode(unsigned char in[MB_SIZE][MB_SIZE][3], unsigned char out[MB_SIZE][MB_SIZE][3])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            float temp[3];
            temp[0] = ( 0.299    * in[x][y][0]) + ( 0.587    * in[x][y][1]) + ( 0.114    * in[x][y][2]);
            temp[1] = (-0.168736 * in[x][y][0]) + (-0.331264 * in[x][y][1]) + ( 0.5      * in[x][y][2]) + 128;
            temp[2] = ( 0.5      * in[x][y][0]) + (-0.418688 * in[x][y][1]) + (-0.081312 * in[x][y][2]) + 128;

            out[x][y][0] =
                (temp[0] > 255) ? 255 :
                (temp[0] < 0) ? 0 :
                temp[0];

            out[x][y][1] =
                (temp[1] > 255) ? 255 :
                (temp[1] < 0) ? 0 :
                temp[1];

            out[x][y][2] =
                (temp[2] > 255) ? 255 :
                (temp[2] < 0) ? 0 :
                temp[2];

            out[x][y][0] &= 0xFC;
            out[x][y][1] &= 0xF8;
            out[x][y][2] &= 0xF8;
        }
    }
}

void yuv_decode(float in[MB_SIZE][MB_SIZE][4], float out[MB_SIZE][MB_SIZE][4])
{
    for(int x = 0; x < MB_SIZE; x++)
    {
        for(int y = 0; y < MB_SIZE; y++)
        {
            out[x][y][0] = in[x][y][0]                                     + ( 1.402    * (in[x][y][2] - 128));
            out[x][y][1] = in[x][y][0] + (-0.344136 * (in[x][y][1] - 128)) + (-0.714136 * (in[x][y][2] - 128));
            out[x][y][2] = in[x][y][0] + ( 1.772    * (in[x][y][1] - 128));

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
