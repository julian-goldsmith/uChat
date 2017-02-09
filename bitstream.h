#ifndef BITSTREAM_H_INCLUDED
#define BITSTREAM_H_INCLUDED

#include <malloc.h>
#include <stdbool.h>
#include "array.h"

typedef struct
{
    array_uint8_t* array;
    unsigned int pos;
} bitstream_t;

inline bitstream_t* bitstream_create()
{
    bitstream_t* bs = (bitstream_t*) malloc(sizeof(bitstream_t));
    bs->array = array_uint8_create(4);
    bs->pos = 0;
    return bs;
}

inline void bitstream_free(bitstream_t* bs)
{
    array_uint8_free(bs->array);
    free(bs);
}

#define BS_ORMASK(n) (1 << (n%8))
#define BS_ANDMASK(n) (~BS_ORMASK(n))

inline void bitstream_append(bitstream_t* bs, bool value)
{
    if(bs->pos % 8 == 0)
    {
        unsigned char c = 0;
        array_uint8_append(bs->array, c);
    }

    unsigned char data = bs->array->base[bs->pos / 8];

    data = (data & BS_ANDMASK(bs->pos)) | (value << (bs->pos%8));
    bs->array->base[bs->pos / 8] = data;

    bs->pos++;
}

inline void bitstream_array_adjust(bitstream_t* bs)
{
    bs->array->len = bs->pos / 8 + 1;
}

inline bool bitstream_peek(bitstream_t* bs)
{
    unsigned char data = bs->array->base[(bs->pos-1) / 8] & BS_ORMASK((bs->pos-1));

    return (bool) (data != 0 ? true : false);
}

inline bool bitstream_read(bitstream_t* bs)
{
    bs->pos++;

    bool temp = bitstream_peek(bs);

    return temp;
}

inline bool bitstream_pop(bitstream_t* bs)
{
    bool temp = bitstream_peek(bs);

    bs->pos--;

    return temp;
}

#endif // BITSTREAM_H_INCLUDED
