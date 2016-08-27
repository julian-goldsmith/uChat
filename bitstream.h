#ifndef BITSTREAM_H_INCLUDED
#define BITSTREAM_H_INCLUDED

#include <malloc.h>
#include <stdbool.h>

typedef struct
{
    unsigned int capacityBytes;
    unsigned int pos;
    unsigned char* data;
} bitstream_t;

inline bitstream_t* bitstream_create()
{
    bitstream_t* bs = (bitstream_t*) calloc(1, sizeof(bitstream_t));
    bs->data = (unsigned char*) calloc(64, 1);
    bs->capacityBytes = 64;
    bs->pos = 0;
    return bs;
}

#define BS_ORMASK(n) (1 << (n%8))
#define BS_ANDMASK(n) (~BS_ORMASK(n))

inline void bitstream_append(bitstream_t* bs, bool value)
{
    if(bs->pos >= bs->capacityBytes * 8)
    {
        // FIXME: resize
        return;
    }

    unsigned char data = bs->data[bs->pos / 8];
    data = (data & BS_ANDMASK(bs->pos)) | (value * BS_ORMASK(value));
    bs->data[bs->pos / 8] = data;
    pos++;
}

inline bool bitstream_read(bitstream_t* ps)
{
    unsigned char data = bs->data[bs->pos / 8];
    data &= BS_ANDMASK(bs->pos);
    data >>= (bs->pos % 8);
    return (bool) data;
}

#endif // BITSTREAM_H_INCLUDED
