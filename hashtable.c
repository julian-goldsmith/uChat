#include <stdbool.h>
#include "array.h"
#include "hashtable.h"

hash_table_t* ht_create()
{
    hash_table_t* ht = (hash_table_t*) malloc(sizeof(hash_table_t));

    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        ht->buckets[i] = malloc(sizeof(bucket_t));
        ht->buckets[i]->len = 0;
        ht->buckets[i]->keys = NULL;
        ht->buckets[i]->vals = NULL;
    }

    return ht;
}

void ht_free(hash_table_t* ht, bool free_keys)
{
    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        bucket_t* bucket = ht->buckets[i];

        if(bucket->keys != NULL)
        {
            if(free_keys)
            {
                for(int j = 0; j < bucket->len; j++)
                {
                    array_free(bucket->keys[j]);
                }
            }

            free(bucket->keys);
        }

        if(bucket->vals != NULL)
        {
            free(bucket->vals);
        }

        free(bucket);
    }

    free(ht);
}

unsigned int hash_func(array_t* array)
{
    unsigned int val = array->len;

    for(int i = 0; i < array->len; i++)
    {
        val ^= array->base[i] << (8 * (i % 4));
    }

    return val;
}

void ht_add(hash_table_t* ht, array_t* key, unsigned int val)
{
    unsigned int bi = hash_func(key) % HT_NUM_BUCKETS;
    bucket_t* bucket = ht->buckets[bi];
    unsigned int blen = bucket->len;

    bucket->keys = realloc(bucket->keys, (blen + 1) * sizeof(array_t*));
    bucket->vals = realloc(bucket->vals, (blen + 1) * sizeof(unsigned int));

    bucket->keys[blen] = key;
    bucket->vals[blen] = val;

    bucket->len = blen + 1;
}

int ht_get(hash_table_t* ht, array_t* key)
{
    unsigned int bi = hash_func(key) % HT_NUM_BUCKETS;
    bucket_t* bucket = ht->buckets[bi];

    for(int i = 0; i < bucket->len; i++)
    {
        array_t* bk = bucket->keys[i];

        if(bk->len == key->len && !memcmp(bk->base, key->base, bk->len))
        {
            return bucket->vals[i];
        }
    }

    return -1;
}
