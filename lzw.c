#include <stdbool.h>
#include "array.h"

array_t* lz_build_initial_dictionary()
{
    array_t* entries = array_create(sizeof(array_t), 512);

    for(int i = 0; i < 256; i++)
    {
        char c = (char) i;

        array_t* item = array_create(1, 1);
        array_append(item, &c);

        array_append(entries, item);
    }

    return entries;
}

bool lz_check_entries(array_t* entries, array_t* new_val)
{
    int i = 0;
    for(array_t* entry = (array_t*) entries->base; entry < ((array_t*) entries->base) + entries->len; entry++)
    {
        if(entry->len == new_val->len && !memcmp(entry->base, new_val->base, entry->len))
        {
            return true;
        }
        i++;
    }

    return false;
}

short lz_get_code(array_t* entries, array_t* encoded)
{
    int i = 0;
    for(array_t* entry = (array_t*) entries->base; entry < ((array_t*) entries->base) + entries->len; entry++)
    {
        if(entry->len == encoded->len && !memcmp(entry->base, encoded->base, entry->len))
        {
            return i;
        }

        i++;
    }

    return false;
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
    array_t* entries = lz_build_initial_dictionary();
    array_t* out_values = array_create(sizeof(short), file_len / 4);

    array_t* encoded = array_create(1, 5);

    for(unsigned char* pos = file_data; pos < file_data + file_len; pos++)
    {
        array_t* new_val = array_copy(encoded);
        array_append(new_val, pos);

        if(lz_check_entries(entries, new_val))
        {
            array_clear(encoded);
            array_append_array(encoded, new_val);
            array_free(new_val);
        }
        else
        {
            array_append(entries, new_val);

            short code = lz_get_code(entries, encoded);
            array_append(out_values, &code);

            array_clear(encoded);
            array_append(encoded, pos);
        }
    }

    short code = lz_get_code(entries, encoded);
    array_append(out_values, &code);

    lz_entries_clean_up(entries);     // FIXME: this segfaults

    return out_values;
}

array_t* lz_decode(array_t* enc_data)
{
    array_t* out_bytes = array_create(1, enc_data->len * 4);
    array_t* entries = lz_build_initial_dictionary();

    short prev_code = *(short*) array_get(enc_data, 0);
    array_append(out_bytes, (unsigned char*) array_get((array_t*) array_get(entries, prev_code), 0));

    for(short* data = ((short*) enc_data->base) + 1; data < ((short*) enc_data->base) + enc_data->len; data++)
    {
        if(*data == entries->len)
        {
            array_t* prev = (array_t*) array_get(entries, prev_code);

            array_t* entry = (array_t*) array_get(entries, prev_code);

            array_t* val = array_copy(prev);
            array_append_array(val, entry);

            array_append(entries, val);
        }
        else
        {
            array_t* prev = (array_t*) array_get(entries, prev_code);

            array_t* entry = (array_t*) array_get(entries, *data);

            array_t* val = array_copy(prev);
            array_append(val, (unsigned char*) array_get(entry, 0));

            array_append(entries, val);
        }

        array_t* entry = (array_t*) array_get(entries, *data);
        array_append_array(out_bytes, entry);
        prev_code = *data;
    }

    lz_entries_clean_up(entries);

    return out_bytes;
}
