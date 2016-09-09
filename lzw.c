#include <stdbool.h>
#include "array.h"
#include "lzw.h"
#include "hashtable.h"

array_t* lz_build_initial_dictionary()
{
    array_t* entries = array_create(sizeof(array_t), 256);

    for(int i = 0; i < 256; i++)
    {
        char c = (char) i;

        array_t* item = array_create(1, 1);
        array_append(item, &c);

        array_append(entries, item);
        free(item);
    }

    return entries;
}

void lz_entries_clean_up(array_t* entries)
{
    for(int i = 0; i < entries->len; i++)
    {
        array_t* entry = array_get(entries, i);
        free(entry->base);
    }

    array_free(entries);
}

array_t* lz_encode(unsigned char* file_data, int file_len)
{
    // leaks solved with Dr. Memory
    array_t* out_values = array_create(sizeof(short), file_len);
    hash_table_t* ht = ht_create();
    unsigned int code_pos = 0;

    for(code_pos = 0; code_pos < 256; code_pos++)
    {
        char c = (char) code_pos;

        array_t* item = array_create(1, 1);
        array_append(item, &c);

        ht_add(ht, item, code_pos);
    }

    array_t* encoded = array_create(1, 5);

    int a = SDL_GetTicks();
    for(unsigned char* pos = file_data; pos < file_data + file_len; pos++)
    {
        while(ht_get_(ht, encoded, *pos) != -1 && pos < file_data + file_len)
        {
            array_append(encoded, pos);
            pos++;
        }

        short code = ht_get(ht, encoded);
        array_append(out_values, &code);

        array_append(encoded, pos);
        ht_add(ht, encoded, code_pos++);

        encoded = array_create(1, 5);
        array_append(encoded, pos);
    }

    short code = ht_get(ht, encoded);
    array_append(out_values, &code);

    ht_free(ht, true);
    array_free(encoded);

    return out_values;
}

array_t* lz_decode(array_t* enc_data)
{
    // leaks solved with Dr. Memory
    array_t* out_bytes = array_create(1, enc_data->len * 4);
    array_t* entries = lz_build_initial_dictionary();

    short prev_code = *(short*) array_get(enc_data, 0);
    array_append(out_bytes, (unsigned char*) array_get((array_t*) array_get(entries, prev_code), 0));

    for(short* data = ((short*) enc_data->base) + 1; data < ((short*) enc_data->base) + enc_data->len; data++)
    {
        assert(*data >= 0);
        array_t* prev = (array_t*) array_get(entries, prev_code);
        array_t* entry = (array_t*) array_get(entries, *data);

        if(*data == entries->len)
        {
            entry = prev;
        }

        array_t* val = array_copy(prev);
        array_append(val, array_get(entry, 0));

        array_append(entries, val);
        free(val);

        array_t* out_entry = (array_t*) array_get(entries, *data);
        array_append_array(out_bytes, out_entry);
        prev_code = *data;
    }

    lz_entries_clean_up(entries);

    return out_bytes;
}
