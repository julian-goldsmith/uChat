#include <stdlib.h>
#include "array.h"
#include "bwt.h"

static void bwt_gen_rotations(const short* inarr, short* rots[256], short* srots[256], short storage[512])
{
    memcpy(storage, inarr, sizeof(short) * 256);
    memcpy(storage + 256, inarr, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        short* tb = storage + i;

        rots[i] = tb;
        srots[i] = tb;
    }
}

int bwt_sort_shorts(const void* p, const void* q)
{
    const short* x = *(const short**) p;
    const short* y = *(const short**) q;

    for(int i = 0; i < 256; i++)
    {
        if(x[i] < y[i])
        {
            return 1;
        }
        else if(x[i] > y[i])
        {
            return -1;
        }
    }

    return 0;
}

static void bwt_merge_sort(short* srots[256], int left, int right, short* scratch[256])
{
    return;
    if(right == left + 1)
    {
        return;
    }

    int length = right - left;
    int midpoint_distance = length / 2;
    int l = left;
    int r = left + midpoint_distance;

    bwt_merge_sort(srots, left, left + midpoint_distance, scratch);
    bwt_merge_sort(srots, left + midpoint_distance, right, scratch);

    for(int i = 0; i < length; i++)
    {
        if(l < left + midpoint_distance && (r == right || srots[l] <srots[r]))
        {
            scratch[i] = srots[l];
        }
        else
        {
            scratch[i] = srots[r];
        }
    }

    for(int i = left; i < right; i++)
    {
        srots[i] = scratch[i - left];
    }
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
    short storage[512];
    short* rots[256];
    short* srots[256];
    short* scratch[256];

    bwt_gen_rotations(inarr, rots, srots, storage);
    bwt_merge_sort(srots, 0, 256, scratch);

    //qsort(srots, 256, sizeof(short*), bwt_sort_shorts);

    for(int i = 0; i < 256; i++)
    {
        int index1 = index_of(rots, srots[i]);
        index1 = (index1 < 255) ? index1 + 1 : 0;

        indexlist[i] = index_of(srots, rots[index1]);

        outarr[i] = srots[i][255];
    }
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
