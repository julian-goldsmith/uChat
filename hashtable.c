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
        ht->buckets[i]->keys = array_create(sizeof(array_t), 5);
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

        for(int j = 0; j < bucket->keys->len; j++)
        {
            array_pool_release(*(array_t**) array_get(bucket->keys, j));
        }

        array_free(bucket->keys);
        array_free(bucket->vals);

        free(bucket);
    }

    free(ht);
}

unsigned fnv_hash_1a_32 (void *key, int len, unsigned char last)
{
    unsigned char *p = key;
    unsigned h = 0x811c9dc5;
    int i;

    for ( i = 0; i < len; i++ )
      h = ( h ^ p[i] ) * 0x01000193;

    h = ( h ^ last ) * 0x01000193;

   return h;
}

void ht_add(hash_table_t* ht, array_t** key, unsigned int val);
int ht_get(hash_table_t* ht, array_t* key);
