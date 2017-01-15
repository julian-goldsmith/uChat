#include <stdlib.h>
#include "array.h"

short** bwt_gen_rotations(const short* inarr)
{
    // returns an array of pointers to shorts.  FIXME: will probably need to do MB_SIZE instead of MB_SIZE*MB_SIZE
    short** outarr = (short**) calloc(256, sizeof(short*));//array_create(sizeof(short*), 256);
    short* block = (short*) calloc(256 * 256, sizeof(short));

    for(int i = 0; i < 256; i++)
    {
        short* tb = block + 256 * i;

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
    short* outarr = (short*) calloc(256, sizeof(short));
    int pos = 0;

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

void shift_rot(short* rot, int j)
{
    for(int i = 255 - j; i > 0; i--)
    {
        rot[i] = rot[i - 1];
    }
}

short* bwt_decode(short* inarr, int inpos)
{
    short** rots = (short**) calloc(256, sizeof(short*));
    short* block = (short*) calloc(256 * 256, sizeof(short));

    for(int i = 0; i < 256; i++)
    {
        rots[i] = block + 256 * i;
    }

    for(int i = 0; i < 256; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            short* rot = rots[j];
            shift_rot(rot, j);
            rot[0] = inarr[j];                                  // inarr is an array of shorts
        }

        qsort(rots, 256, sizeof(short*), bwt_sort_shorts);
    }

    return rots[inpos];     // FIXME: leak
}
