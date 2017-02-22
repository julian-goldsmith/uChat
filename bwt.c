#include <stdlib.h>
#include "array.h"
#include "bwt.h"

short** bwt_gen_rotations(const short* inarr)
{
    // FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) malloc(256 * sizeof(short*));
    short* block = (short*) malloc(2 * 256 * sizeof(short));

    memcpy(block, inarr, sizeof(short) * 256);
    memcpy(block + 256, inarr, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        short* tb = block + 256 - i;

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
    short* outarr = (short*) calloc(256, sizeof(short));
    short pos = 0;

    short** rots = bwt_gen_rotations(inarr);       // array of short*s
    qsort(rots, 256, sizeof(short*), bwt_sort_shorts);

    for(int i = 0; i < 256; i++)
    {
        outarr[i] = rots[i][255];
    }

    for(int i = 0; i < 256; i++)
    {
        if(!memcmp(rots[i], inarr, sizeof(short) * 256))   // rots is arr of short*s
        {
            pos = i;
            break;
        }
    }

    *posp = pos;

    return outarr;
}

short* bwt_decode(short* outs, unsigned short pos)
{
    short storage[256][256];
    short* rots[256];                // we need short*s, since qsort changes them in-place.  thank you Stragus from Freenode's ##c

    for(int i = 0; i < 256; i++)
    {
        rots[i] = storage[i];
    }

    for(int i = 0; i < 256; i++)
    {
        for(int row = 0; row < 256; row++)
        {
            for(int j = 254; j >= 0; j--)
            {
                rots[row][j + 1] = rots[row][j];
            }

            rots[row][0] = outs[row];
        }

        qsort(rots, 256, sizeof(short*), bwt_sort_shorts);
    }

    short* outv = (short*) malloc(512);
    memcpy(outv, rots[pos], 512);

    return outv;
}
