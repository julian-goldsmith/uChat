#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <malloc.h>
#include <memory.h>
#include <assert.h>

typedef struct
{
    unsigned int len;
    unsigned int capacity;
    unsigned int item_size;
    unsigned char* base;
} array_t;

inline array_t* array_create(unsigned int item_size, unsigned int initial_capacity)
{
    array_t* array = (array_t*) malloc(sizeof(array_t));

    array->len = 0;
    array->capacity = initial_capacity;
    array->item_size = item_size;
    array->base = (unsigned char*) malloc(array->capacity * array->item_size);

    return array;
}

inline array_t* array_create_from_pointer(void* pointer, unsigned int item_size, unsigned int len)
{
    array_t* array = (array_t*) malloc(sizeof(array_t));

    // FIXME: copy data at pointer instead of using like this
    array->len = len;
    array->capacity = len;
    array->item_size = item_size;
    array->base = pointer;

    return array;
}

inline void array_free(array_t* array)
{
    free(array->base);
    free(array);
}

inline void* array_get(array_t* array, unsigned int idx)
{
    return (void*) (array->base + idx * array->item_size);
}

inline void array_set(array_t* array, unsigned int idx, void* item)
{
    if(idx >= array->len)
    {
        // FIXME: shit the bed
        return;
    }

    memcpy(array->base + idx * array->item_size, item, array->item_size);
}

inline unsigned int array_append(array_t* array, void* item)
{
    if(array->capacity == array->len)
    {
        array->base = (unsigned char*) realloc(array->base, (array->capacity + 5) * array->item_size);
    }

    array->len++;
    array_set(array, array->len - 1, item);

    return array->len - 1;
}

inline array_t* array_copy(array_t* array)
{
    array_t* new_array = array_create(array->item_size, array->capacity);

    new_array->len = array->len;
    memcpy(new_array->base, array->base, array->len * array->item_size);

    return new_array;
}

inline array_t* array_append_array(array_t* array1, array_t* array2)
{
    assert(array1->item_size == array2->item_size);

    // FIXME: don't blindly add capacities, check if they can both fit in 1
    array_t* new_array = array_create(array1->item_size, array1->capacity + array2->capacity);

    new_array->len = array1->len;
    memcpy(new_array->base, array1->base, array1->len * array1->item_size);

    new_array->len += array2->len;
    memcpy(new_array->base + array1->len * array1->item_size, array2->base, array2->len * array2->item_size);

    return new_array;
}

#endif // ARRAY_H_INCLUDED
