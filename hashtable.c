#include <stdbool.h>
#include "array.h"
#include "hashtable.h"
#include "arraypool.h"

hash_table_t* ht_create()
{
    hash_table_t* ht = (hash_table_t*) malloc(sizeof(hash_table_t));

    for(unsigned int i = 0; i < HT_NUM_BUCKETS; i++)
    {
        ht->buckets[i] = malloc(sizeof(bucket_t));
        ht->buckets[i]->keys = array_create(sizeof(uint64_t), 5);
        ht->buckets[i]->vals = array_create(sizeof(unsigned int), 5);
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

        array_free(bucket->keys);
        array_free(bucket->vals);

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

        array_clear(bucket->keys);
        array_clear(bucket->vals);
    }
}

uint64_t fnv_hash (void *key, int len)
{
    unsigned char *p = key;
    uint64_t h = 0xcbf29ce484222325;
    int i;

    for ( i = 0; i < len; i++ )
      h = ( h ^ p[i] ) * 0x100000001b3;

   return h;
}

void ht_add(hash_table_t* ht, array_t* key, unsigned int val);
int ht_get(hash_table_t* ht, array_t* key);
