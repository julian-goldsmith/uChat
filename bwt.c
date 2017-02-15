#include <stdlib.h>
#include "array.h"
#include "bwt.h"

short** bwt_gen_rotations(const short* inarr)
{
    // FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) malloc(16 * sizeof(short*));
    short* block = (short*) malloc(2 * 16 * sizeof(short));

    memcpy(block, inarr, sizeof(short) * 16);
    memcpy(block + 16, inarr, sizeof(short) * 16);

    for(int i = 0; i < 16; i++)
    {
        short* tb = block + 16 - i;

        outarr[i] = tb;
    }

    return outarr;
}

int bwt_sort_shorts(const void* p, const void* q)
{
    const short* x = *(const short**) p;
    const short* y = *(const short**) q;

    for(int i = 0; i < 16; i++)
    {
        if(x[i] < y[i])
        {
            return -1;
        }
        else if(x[i] > y[i])
        {
            return 1;
        }
    }

    return 0;
}

short* bwt_encode(const short* inarr, unsigned short* posp)
{
    short* outarr = (short*) calloc(16, sizeof(short));
    short pos = 0;

    // FIXME: not 100% sure this sort is right
    short** rots = bwt_gen_rotations(inarr);       // array of short*s
    qsort(rots, 16, sizeof(short*), bwt_sort_shorts);

    for(int i = 0; i < 16; i++)
    {
        outarr[i] = rots[i][15];
    }

    for(int i = 0; i < 16; i++)
    {
        if(!memcmp(rots[i], inarr, sizeof(short) * 16))   // rots is arr of short*s
        {
            pos = i;
            break;
        }
    }

    *posp = pos;

    return outarr;
}

short* bwt_decode(short outs[16], unsigned short pos)
{
    short rots[16][16];

    short* fuck[16];
    for(int i = 0; i < 16; i++) fuck[i] = rots[i];

    for(int i = 0; i < 16; i++) for(int j = 0; j < 16; j++) rots[i][j] = 0;

    for(int i = 0; i < 16; i++)
    {
        // insert s as a column of table before the first column of the table
        for(int row = 0; row < 16; row++)
        {
            short temp[16];

            for(int j = 0; j < 16; j++)
            {
                temp[j] = fuck[row][j];
            }

            fuck[row][0] = outs[row];
            for(int j = 0; j < 15; j++)
            {
                fuck[row][j + 1] = temp[j];
            }
        }

        qsort(fuck, 16, sizeof(short*), bwt_sort_shorts);
    }

    short* outv = (short*) malloc(32);
    memcpy(outv, rots[pos], 32);

    return outv;
}
