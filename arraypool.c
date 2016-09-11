#ifndef ARRAYPOOL_H_INCLUDED
#define ARRAYPOOL_H_INCLUDED

#include <stdint.h>
#include "array.h"

// linked list of chunks of arrays
// chunks have bitmap of used arrays

typedef struct pool_chunk_s
{
    array_t** items;
    uint32_t bitmap;
    struct pool_chunk_s* next;
} pool_chunk_t;

pool_chunk_t* root;

pool_chunk_t* array_pool_create_chunk()
{
    pool_chunk_t* chunk = (pool_chunk_t*) malloc(sizeof(pool_chunk_t));
    chunk->bitmap = 0;
    chunk->next = NULL;
    chunk->items = (array_t**) malloc(sizeof(array_t*) * 32);

    for(unsigned int i = 0; i < 32; i++)
    {
        chunk->items[i] = array_create(1, 5);
    }

    return chunk;
}

void array_pool_init()
{
    root = array_pool_create_chunk();

    pool_chunk_t* prev = root;
    for(int i = 0; i < 8192; i++)
    {
        pool_chunk_t* pos = array_pool_create_chunk();
        prev->next = pos;
        prev = pos;
    }
}

array_t* array_pool_get()
{
    for(pool_chunk_t* pos = root; pos != NULL; pos = pos->next)
    {
        if(pos->bitmap == 0xFFFF)
        {
            continue;
        }

        for(int i = 0; i < 32; i++)
        {
            if((pos->bitmap & (1 << i)) == 0)
            {
                pos->bitmap |= 1 << i;
                return pos->items[i];
            }
        }
    }

    assert(0);
    return NULL;
}

void array_pool_release(array_t* item)
{
    for(pool_chunk_t* pos = root; pos != NULL; pos = pos->next)
    {
        for(int i = 0; i < 32; i++)
        {
            if(pos->items[i] == item)
            {
                pos->bitmap &= ~(1 << i);

                array_clear(item);

                return;
            }
        }
    }

    assert(0);
}

#endif // ARRAYPOOL_H_INCLUDED
