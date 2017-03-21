#include <stdlib.h>
#include "array.h"
#include "bwt.h"

short** bwt_gen_rotations(const short* inarr)
{
    // FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) malloc(256 * sizeof(short*));
    short* block = (short*) malloc(2 * 256 * sizeof(short));        // FIXME: block leaks

    memcpy(block, inarr, sizeof(short) * 256);
    memcpy(block + 256, inarr, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        short* tb = block + i;

        outarr[i] = tb;
    }

    return outarr;
}

int bwt_sort_shorts(const void* p, const void* q)
{
    const short* x = *(const short**) p;
    const short* y = *(const short**) q;

    /*for(int i = 15; i >= 0; i--)
    {
        if(x[i] < y[i])
        {
            return -1;
        }
        else if(x[i] > y[i])
        {
            return 1;
        }
    }*/

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

static int index_of(short* table[256], short search[256])
{
    for(int i = 0; i < 256; i++)
    {
        if(!memcmp(table[i], search, 512))
        {
            return i;
        }
    }

    assert(false);
    return -1;
}

void bwt_encode(const short* inarr, short outarr[256], short indexlist[256])
{
    short** rots = bwt_gen_rotations(inarr);       // array of short*s
    short* srots[256];

    for(int i = 0; i < 256; i++)
    {
        srots[i] = rots[i];
    }

    //qsort(srots, 256, sizeof(short*), bwt_sort_shorts);

    for(int i = 0; i < 256; i++)
    {
        int index1 = index_of(rots, srots[i]);
        index1 = (index1 < 255) ? index1 + 1 : 0;

        indexlist[i] = index_of(srots, rots[index1]);
    }

    for(int i = 0; i < 256; i++)
    {
        outarr[i] = srots[i][255];
    }

    short decoded[256];
    bwt_decode(outarr, indexlist, decoded);
    assert(!memcmp(inarr, decoded, sizeof(decoded)));
}

void bwt_decode(short outs[256], short indexlist[256], short outv[256])
{
    int x = indexlist[0];
    for(int i = 0; i < 256; i++)
    {
        outv[i] = outs[x];
        x = indexlist[x];
    }
}
