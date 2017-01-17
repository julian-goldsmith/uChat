#include <stdbool.h>
#include "array.h"
#include "hashtable.h"

hash_table_t* ht_create()
{
    hash_table_t* ht = (hash_table_t*) malloc(sizeof(hash_table_t));

    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        ht->buckets[i] = malloc(sizeof(bucket_t));
        ht->buckets[i]->keys = array_uint64_create(5);
        ht->buckets[i]->vals = array_uint_create(5);
    }

    return ht;
}

void ht_free(hash_table_t* ht)
{
    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        bucket_t* bucket = ht->buckets[i];

        assert(bucket->vals != NULL);
        assert(bucket->keys != NULL);

        array_uint64_free(bucket->keys);
        array_uint_free(bucket->vals);

        free(bucket);
    }

    free(ht);
}

void ht_clear(hash_table_t* ht)
{
    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        bucket_t* bucket = ht->buckets[i];

        assert(bucket->vals != NULL);
        assert(bucket->keys != NULL);

        array_uint64_clear(bucket->keys);
        array_uint_clear(bucket->vals);
    }
}

uint64_t fnv_hash (void *key, int len)
{
    uint64_t h = 0xcbf29ce484222325;

    for(unsigned char *p = key; p < (unsigned char*) key + len; p++)
    {
        h = (h ^ *p) * 0x100000001b3;
    }

    return h;
}

void ht_add(hash_table_t* ht, array_uint8_t* key, unsigned int val);
int ht_get(hash_table_t* ht, array_uint8_t* key);
int ht_get2(hash_table_t* ht, uint64_t hash);
