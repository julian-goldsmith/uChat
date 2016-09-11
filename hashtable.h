#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#include <stdint.h>

#define HT_NUM_BUCKETS 8192     // must be a power of 2

typedef struct
{
    array_t* keys;
    array_t* vals;
} bucket_t;

typedef struct
{
    bucket_t* buckets[HT_NUM_BUCKETS];
} hash_table_t;

hash_table_t* ht_create();
void ht_free(hash_table_t* ht);
unsigned int hash_func(array_t* array);
uint64_t fnv_hash (void *key, int len);

inline void ht_add(hash_table_t* ht, array_t* key, unsigned int val)
{
    uint64_t hash = fnv_hash(key->base, key->len);
    unsigned int bi = hash & (HT_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    array_append(bucket->keys, &hash);
    array_append(bucket->vals, &val);
}

inline int ht_get(hash_table_t* ht, array_t* key)
{
    uint64_t hash = fnv_hash(key->base, key->len);
    unsigned int bi = hash & (HT_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    for(int i = 0; i < bucket->keys->len; i++)
    {
        uint64_t bk = *(uint64_t*) array_get(bucket->keys, i);

        if(bk == hash)
        {
            return *(int*) array_get(bucket->vals, i);
        }
    }

    return -1;
}

#endif // HASHTABLE_H_INCLUDED
