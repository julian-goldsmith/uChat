#include <stdbool.h>
#include "array.h"
#include "lzw.h"
#include "hashtable.h"
#include "arraypool.h"

array_define_type(array_uint8_tp, array_uint8_t*)
array_implement_type(array_uint8_tp, array_uint8_t*)

array_array_uint8_tp_t* lz_build_initial_dictionary()
{
    array_array_uint8_tp_t* entries = array_array_uint8_tp_create(256);

    for(int i = 0; i < 256; i++)
    {
        unsigned char c = (unsigned char) i;

        array_uint8_t* item = array_uint8_create(1);
        array_uint8_append(item, c);

        array_array_uint8_tp_append(entries, item);
    }

    return entries;
}

void lz_entries_clean_up(array_array_uint8_tp_t* entries)
{
    for(int i = 0; i < entries->len; i++)
    {
        array_uint8_t* entry = array_array_uint8_tp_get(entries, i);
        array_uint8_free(entry);
    }

    array_array_uint8_tp_free(entries);
}

hash_table_t* ht;

array_sint16_t* lz_encode(const unsigned char* file_data, int file_len)
{
    array_sint16_t* out_values = array_sint16_create(1024);

    if(ht == NULL)
    {
        ht = ht_create();
    }

    unsigned int code_pos = 0;

    for(code_pos = 0; code_pos < 256; code_pos++)
    {
        array_uint8_t* item = array_pool_get();
        array_uint8_append(item, (unsigned char) code_pos);

        ht_add(ht, item, code_pos);
    }

    for(const unsigned char* pos = file_data; pos < file_data + file_len;)
    {
        array_uint8_t* encoded = array_pool_get();
        short prev_code = -1;

        uint64_t h = 0xcbf29ce484222325;

        while(pos < file_data + file_len)
        {
            h = (h ^ *pos) * 0x100000001b3;

            short new_code = ht_get2(ht, h);

            array_uint8_append(encoded, *pos);

            if(new_code == -1)
            {
                ht_add(ht, encoded, code_pos++);
                break;
            }

            prev_code = new_code;
            pos++;
        }

        assert(prev_code >= 0);
        array_sint16_append(out_values, prev_code);
    }

    ht_clear(ht);
    array_pool_release_all();

    return out_values;
}

array_uint8_t* lz_decode(array_sint16_t* enc_data)
{
    array_uint8_t* entry;
    array_uint8_t* out_bytes = array_uint8_create(enc_data->len * 4);
    array_array_uint8_tp_t* entries = lz_build_initial_dictionary();

    short prev_code = array_sint16_get(enc_data, 0);
    array_uint8_t* prev = array_array_uint8_tp_get(entries, prev_code);
    array_uint8_append(out_bytes, array_uint8_get(prev, 0));

    for(short* data = enc_data->base + 1; data < enc_data->base + enc_data->len; data++)
    {
        assert(*data >= 0);

        prev = (array_uint8_t*) array_array_uint8_tp_get(entries, prev_code);

        if(*data == entries->len)
        {
            entry = prev;
        }
        else
        {
            entry = (array_uint8_t*) array_array_uint8_tp_get(entries, *data);
        }

        array_uint8_t* val = array_uint8_copy(prev);
        array_uint8_append(val, array_uint8_get(entry, 0));

        array_array_uint8_tp_append(entries, val);

        array_uint8_t* out_entry = array_array_uint8_tp_get(entries, *data);
        array_uint8_append_array(out_bytes, out_entry);

        prev_code = *data;
    }

    lz_entries_clean_up(entries);

    return out_bytes;
}
