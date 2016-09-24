#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <stdlib.h>

typedef struct
{
    unsigned int len;
    unsigned int capacity;
    unsigned int item_size;
    unsigned char* base;
    unsigned short magic;
} array_t;

inline array_t* array_create(unsigned int item_size, unsigned int initial_capacity)
{
    array_t* array = (array_t*) malloc(sizeof(array_t));

    array->len = 0;
    array->capacity = initial_capacity;
    array->item_size = item_size;
    array->base = (unsigned char*) malloc(array->capacity * array->item_size);
    array->magic = 0x1234;

    return array;
}

inline array_t* array_create_from_pointer(void* pointer, unsigned int item_size, unsigned int len)
{
    array_t* array = (array_t*) malloc(sizeof(array_t));

    // FIXME: copy data at pointer instead of using like this
    array->len = len;
    array->capacity = len;
    array->item_size = item_size;
    array->base = (unsigned char*) pointer;

    return array;
}

inline void array_free(array_t* array)
{
    if(array->magic != 0x1234)
    {
        assert(array->magic == 0x1234);
    }
    free(array->base);
    free(array);
}

inline void* array_get(array_t* array, unsigned int idx)
{
    return (void*) (array->base + idx * array->item_size);
}

inline void array_set(array_t* array, unsigned int idx, void* item)
{
    assert(idx < array->len);

    memcpy(array->base + idx * array->item_size, item, array->item_size);
}

inline unsigned int array_append(array_t* array, void* item)
{
    if(array->capacity == array->len)
    {
        array->capacity *= 2;
        array->base = (unsigned char*) realloc(array->base, array->capacity * array->item_size);
    }

    array->len++;

    if(item != NULL)
    {
        array_set(array, array->len - 1, item);
    }

    return array->len - 1;
}

inline void array_appendm(array_t* array, void* item, unsigned int count)
{
    if(array->capacity <= array->len + count)
    {
        array->capacity *= 2;

        if(array->capacity <= array->len + count)
        {
            array->capacity += count;
        }

        array->base = (unsigned char*) realloc(array->base, array->capacity * array->item_size);
    }

    memcpy(array->base + array->len * array->item_size, item, count * array->item_size);

    array->len += count;
}

inline void* array_get_new(array_t* array)
{
    unsigned int idx = array_append(array, NULL);
    return (void*) (array->base + idx * array->item_size);
}

inline array_t* array_copy(array_t* array)
{
    array_t* new_array = array_create(array->item_size, array->capacity);

    new_array->len = array->len;
    memcpy(new_array->base, array->base, array->len * array->item_size);

    return new_array;
}

inline void array_append_array(array_t* array1, array_t* array2)
{
    assert(array1->item_size == array2->item_size);

    if(array1->capacity <= array1->len + array2->len)
    {
        array1->capacity *= 2;

        if(array1->capacity <= array1->len + array2->len)
        {
            array1->capacity += array2->len;
        }

        array1->base = (unsigned char*) realloc(array1->base, array1->capacity * array1->item_size);
    }

    memcpy(array1->base + array1->len * array1->item_size, array2->base, array2->len * array2->item_size);
    array1->len += array2->len;
}

inline void array_pop(array_t* array)
{
    assert(array->len > 0);
    array->len--;
}

inline void array_clear(array_t* array)
{
    array->len = 0;
}

#endif // ARRAY_H_INCLUDED
