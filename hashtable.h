#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

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
unsigned fnv_hash_1a_32 (void *key, int len, unsigned char last);

inline void ht_add(hash_table_t* ht, array_t** key, unsigned int val)
{
    unsigned int bi = fnv_hash_1a_32((*key)->base, (*key)->len - 1, (*key)->base[(*key)->len - 1]) & (HT_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    array_append(bucket->keys, key);
    array_append(bucket->vals, &val);
}

inline int ht_get(hash_table_t* ht, array_t* key)
{
    unsigned int bi = fnv_hash_1a_32(key->base, key->len - 1, key->base[key->len - 1]) & (HT_NUM_BUCKETS - 1);
    bucket_t* bucket = ht->buckets[bi];

    for(int i = 0; i < bucket->keys->len; i++)
    {
        array_t* bk = * (array_t**) array_get(bucket->keys, i);

        if(bk->len == key->len && !memcmp(bk->base, key->base, bk->len))
        {
            return *(int*) array_get(bucket->vals, i);
        }
    }

    return -1;
}

#endif // HASHTABLE_H_INCLUDED
