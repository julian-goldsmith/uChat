#ifndef ARRAYPOOL_H_INCLUDED
#define ARRAYPOOL_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "array.h"
#include "arraypool.h"

typedef struct pool_chunk_s
{
    array_t** items;
    uint32_t bitmap;
    struct pool_chunk_s* next;
} pool_chunk_t;

// linked list of chunks of arrays
// chunks have bitmap of used arrays

pool_chunk_t* root;
pool_chunk_t* curr_free;

pool_chunk_t* array_pool_create_chunk()
{
    pool_chunk_t* chunk = (pool_chunk_t*) malloc(sizeof(pool_chunk_t));
    chunk->bitmap = 0;
    chunk->next = NULL;
    chunk->items = (array_t**) malloc(sizeof(array_t*) * 32);

    for(unsigned int i = 0; i < 32; i++)
    {
        chunk->items[i] = array_create(1, 25);
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

    curr_free = root;
}

array_t* array_pool_get()
{
    if(curr_free->bitmap == 0xFFFF)
    {
        for(curr_free = root;
            curr_free != NULL && curr_free->bitmap == 0xFFFF;
            curr_free = curr_free->next);
    }

    assert(curr_free != NULL);

    for(int i = 0; i < 32; i++)
    {
        if((curr_free->bitmap & (1 << i)) == 0)
        {
            curr_free->bitmap |= 1 << i;
            return curr_free->items[i];
        }
    }

    assert(0);
    return NULL;
}

void array_pool_release_all()
{
    for(pool_chunk_t* pos = root; pos != NULL; pos = pos->next)
    {
        if(pos->bitmap == 0) continue;

        for(int i = 0; i < 32; i++)
        {
            array_clear(pos->items[i]);
        }

        pos->bitmap = 0;
    }
}

#endif // ARRAYPOOL_H_INCLUDED
