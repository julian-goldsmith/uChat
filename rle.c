#include <malloc.h>
#include "imgcoder.h"

float* rle_encode_block(float data[MB_SIZE][MB_SIZE][4], int* rleSize)
{
    float* odata = (float*) calloc(MB_SIZE * MB_SIZE * 4, sizeof(float));    // resize later
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

void rle_decode_block(float data[MB_SIZE][MB_SIZE][4], float* rleData, int rleSize)
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
