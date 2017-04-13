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

int bwt_sort_shorts(const short* x, const short* y)
{
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

static int bwt_quicksort_partition(short** srots, int len)
{
    int i = 0;

    for(int j = 0; j < len; j++)
    {
        if(bwt_sort_shorts(srots[j], srots[0]))
        {
            i++;

            short* temp = srots[j];
            srots[j] = srots[i];
            srots[i] = temp;
        }
    }

    short* temp = srots[len - 1];
    srots[len - 1] = srots[i];
    srots[i] = temp;

    return i;
}

static void bwt_quicksort(short** srots, int len)
{
    if(len == 1)
    {
        return;
    }

    int part_idx = bwt_quicksort_partition(srots, len);

    bwt_quicksort(srots, part_idx);
    bwt_quicksort(srots + part_idx, len - part_idx);
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
    //short* scratch[256];

    bwt_gen_rotations(inarr, rots, srots, storage);
    //bwt_merge_sort(srots, 0, 256, scratch);
    bwt_quicksort(srots, 256);

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
