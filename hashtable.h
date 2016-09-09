#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

#define HT_NUM_BUCKETS 2048

typedef struct
{
    unsigned int len;
    array_t** keys;
    unsigned int* vals;
} bucket_t;

typedef struct
{
    bucket_t* buckets[HT_NUM_BUCKETS];
} hash_table_t;

hash_table_t* ht_create();
void ht_free(hash_table_t* ht, bool free_keys);
unsigned int hash_func(array_t* array);
void ht_add(hash_table_t* ht, array_t* key, unsigned int val);
int ht_get(hash_table_t* ht, array_t* key);
int ht_get_(hash_table_t* ht, array_t* key, unsigned char b);

#endif // HASHTABLE_H_INCLUDED
