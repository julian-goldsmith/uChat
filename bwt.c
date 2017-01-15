#include <stdlib.h>
#include "array.h"

short** bwt_gen_rotations(const short* inarr)
{
    // returns an array of pointers to shorts.  FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) calloc(256, sizeof(short*));//array_create(sizeof(short*), 256);
    short* block = (short*) calloc(256 * 256, sizeof(short));

    for(int i = 0; i < 256; i++)
    {
        short* tb = block + i;

        memcpy(tb, inarr + i, sizeof(short) * (256 - i));
        memcpy(tb, inarr, sizeof(short) * i);

        outarr[i] = tb;
    }

    return outarr;
}

int bwt_sort_shorts(const void* p, const void* q)
{
    const short* x = (const short*) p;
    const short* y = (const short*) q;

    return memcmp(x, y, sizeof(short) * 256);
}

short* bwt_encode(const short* inarr, int* posp)
{
    array_t* outarr = array_create(sizeof(short), 256);
    int pos = 0;

    // FIXME: not 100% sure this sort is right
    short** rots = bwt_gen_rotations(inarr);       // array of short*s
    qsort(array_get(rots, 0), 256, sizeof(short*), bwt_sort_shorts);

    for(int i = 0; i < 256; i++)
    {
        //array_append(outarr, (short*) array_get(rots, i) + 255);
        array_append(outarr, rots[i] + 255);
    }

    for(int i = 0; i < 256; i++)
    {
        if(!memcmp(array_get(rots, i), inarr, sizeof(short) * 256))   // rots is arr of short*s
        {
            pos = i;
            break;
        }
    }

    *posp = pos;

    short* retarr = (short*) outarr->base;
    free(outarr);

    return retarr;
}

short* bwt_decode(short* inarr, int inpos)
{
    array_t* rots = array_create(sizeof(short*), 256);
    // FIXME: memory leaks everywhere

    for(int i = 0; i < 256; i++)
    {
        array_append(rots, (short*) calloc(256, sizeof(short)));
    }

    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            short* rot = (short*) array_get(rots, j);
            memcpy(rot + 1, rot, sizeof(short) * (255 - j));    // FIXME: this behavior is undefined
            rot[0] = inarr[j];                                  // inarr is an array of shorts
        }

        qsort(array_get(rots, 0), 256, sizeof(short*), bwt_sort_shorts);
    }

    return array_get(rots, inpos);
}
