#include <stdlib.h>
#include "array.h"

short** bwt_gen_rotations(const short* inarr)
{
    // FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) calloc(256, sizeof(short*));
    short* block = (short*) calloc(2 * 256, sizeof(short));

    memcpy(block, inarr, sizeof(short) * 256);
    memcpy(block + 256, inarr, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        outarr[i] = block + 256 - i;
    }

    return outarr;
}

int bwt_sort_shorts(const void* p, const void* q)
{
    const short* x = (const short*) p;
    const short* y = (const short*) q;

    int i = 0;

    while(*x++ == *y++ && i < 256) i++;

    return *x < *y ? -1 :
           *x > *y ? 1 :
           0;
}

short* bwt_encode(const short* inarr, int* posp)
{
    short* outarr = (short*) calloc(256, sizeof(short));
    int pos = -1;

    // FIXME: not 100% sure this sort is right
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

void shift_rot(short* rot, int q)
{
    for(int i = q; i > 0; i--)
    {
        rot[i] = rot[i - 1];
    }
}

short* bwt_decode(short* inarr, int inpos)
{
    short** rots = (short**) calloc(256, sizeof(short*));
    short* block = (short*) calloc(2 * 256, sizeof(short));

    memcpy(block, inarr, sizeof(short) * 256);
    memcpy(block + 256, inarr, sizeof(short) * 256);

    for(int i = 0; i < 256; i++)
    {
        rots[i] = block + 256 - i;
    }

    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            short* rot = rots[j];
            shift_rot(rot, i);
            rot[0] = inarr[j];                                  // inarr is an array of shorts
        }

        qsort(rots, 256, sizeof(short*), bwt_sort_shorts);
    }

    return rots[inpos];     // FIXME: leak
}
