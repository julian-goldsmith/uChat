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

#define aht_NUM_BUCKETS 256     // must be a power of 2

typedef struct
{
    array_t* keys;
    array_t* vals;
} bucket_t;

typedef struct
{
    bucket_t* buckets[aht_NUM_BUCKETS];
} hash_table_t;

unsigned fnv_hash_1a_32 (void *key, int len);

void aht_add(hash_table_t* ht, array_t** key, pool_chunk_t** val)
{
    unsigned int bi = fnv_hash_1a_32(key, sizeof(array_t*)) & (aht_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    assert(val != NULL);

    array_append(bucket->keys, key);
    array_append(bucket->vals, val);
}

pool_chunk_t* aht_get(hash_table_t* ht, array_t* key)
{
    unsigned int bi = fnv_hash_1a_32(&key, sizeof(array_t*)) & (aht_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    for(int i = 0; i < bucket->keys->len; i++)
    {
        array_t* bk = *(array_t**) array_get(bucket->keys, i);

        if(bk == key)
        {
            return *(pool_chunk_t**) array_get(bucket->vals, i);
        }
    }

    return NULL;
}

hash_table_t* aht_create()
{
    hash_table_t* ht = (hash_table_t*) malloc(sizeof(hash_table_t));

    for(unsigned int i = 0; i < aht_NUM_BUCKETS; i++)
    {
        ht->buckets[i] = malloc(sizeof(bucket_t));
        ht->buckets[i]->keys = array_create(sizeof(array_t*), 5);
        ht->buckets[i]->vals = array_create(sizeof(pool_chunk_t*), 5);
    }

    return ht;
}

// linked list of chunks of arrays
// chunks have bitmap of used arrays

pool_chunk_t* root;
hash_table_t* aht;
pool_chunk_t* curr_free;

pool_chunk_t* array_pool_create_chunk()
{
    pool_chunk_t* chunk = (pool_chunk_t*) malloc(sizeof(pool_chunk_t));
    chunk->bitmap = 0;
    chunk->next = NULL;
    chunk->items = (array_t**) malloc(sizeof(array_t*) * 32);

    for(unsigned int i = 0; i < 32; i++)
    {
        chunk->items[i] = array_create(1, 5);
        aht_add(aht, &chunk->items[i], &chunk);
    }

    return chunk;
}

void array_pool_init()
{
    aht = aht_create();
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

void array_pool_release(array_t* item)
{
    pool_chunk_t* pos = aht_get(aht, item);
    assert(pos != NULL);

    int i;
    for(i = 0; i < 32 && pos->items[i] != item; i++);

    pos->bitmap &= ~(1 << i);

    array_clear(item);
}

void array_pool_release_all()
{
    for(pool_chunk_t* pos = root; pos != NULL; pos = pos->next)
    {
        if(pos->bitmap == 0) break;

        for(int i = 0; i < 32; i++)
        {
            array_clear(pos->items[i]);
        }

        pos->bitmap = 0;
    }
}

#endif // ARRAYPOOL_H_INCLUDED
