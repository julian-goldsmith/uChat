#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef DEBUG
#include <stdbool.h>
#endif

// FIXME: remove from_pointer when not debugging
// FIXME: allow arrays to be created on stack
//        (i.e. array_xx_create doesn't malloc array_xx_t, takes in pointer)
#define array_define_type(name, type) \
    typedef struct \
    { \
        unsigned int len; \
        unsigned int capacity; \
        type *base; \
        unsigned short magic; \
        bool from_pointer; \
    } array_ ## name ## _t; \
\
inline array_ ## name ## _t* array_ ## name ## _create(unsigned int initial_capacity) \
{ \
    array_ ## name ##_t* array = (array_ ## name ## _t*) malloc(sizeof(array_ ## name ## _t)); \
    \
    array->len = 0; \
    array->capacity = initial_capacity; \
    array->base = (type*) malloc(array->capacity * sizeof(type)); \
    array->magic = 0x1234; \
    \
    array->from_pointer = false; \
    \
    return array; \
} \
\
inline array_ ## name ## _t* array_ ## name ## _create_from_pointer(type* pointer, unsigned int len) \
{ \
    array_ ## name ## _t* array = (array_ ## name ## _t*) malloc(sizeof(array_ ## name ## _t)); \
    \
    /* FIXME: possibly copy data at pointer instead of using like this */ \
    array->len = len; \
    array->capacity = len; \
    array->base = (type*) pointer; \
    \
    array->from_pointer = true; \
    \
    return array; \
} \
\
inline void array_ ## name ## _free(array_ ## name ## _t* array) \
{ \
    assert(array->magic == 0x1234); \
    assert(!array->from_pointer); \
    \
    free(array->base); \
    free(array); \
} \
\
inline type array_ ## name ## _get(array_ ## name ## _t* array, unsigned int idx) \
{ \
    return array->base[idx]; \
} \
\
inline void array_ ## name ## _set(array_ ## name ## _t* array, unsigned int idx, type item) \
{ \
    assert(idx < array->len); \
    \
    memcpy(array->base + idx, &item, sizeof(type)); \
} \
\
inline unsigned int array_ ## name ## _append(array_ ## name ## _t* array, type item) \
{ \
    if(array->capacity == array->len) \
    { \
        array->capacity *= 2; \
        array->base = (type*) realloc(array->base, array->capacity * sizeof(type)); \
    } \
    \
    array->len++; \
    array_ ## name ## _set(array, array->len - 1, item); \
    \
    return array->len - 1; \
} \
\
inline void array_ ## name ## _appendm(array_ ## name ## _t* array, type* item, unsigned int count) \
{ \
    if(array->capacity <= array->len + count) \
    { \
        array->capacity *= 2; \
        \
        if(array->capacity <= array->len + count) \
        { \
            array->capacity += count; \
        } \
        \
        array->base = (type*) realloc(array->base, array->capacity * sizeof(type)); \
    } \
    \
    memcpy(array->base + array->len, item, count * sizeof(type)); \
    \
    array->len += count; \
} \
\
inline array_ ## name ## _t* array_ ## name ## _copy(array_ ## name ## _t* array) \
{ \
    assert(array->capacity > 0); \
    array_ ## name ## _t* new_array = array_ ## name ## _create(array->capacity); \
    assert(new_array->capacity > 0); \
    assert(new_array->base != NULL); \
    \
    new_array->len = array->len; \
    memcpy(new_array->base, array->base, array->len * sizeof(type)); \
    \
    return new_array; \
} \
\
inline void array_ ## name ## _append_array(array_ ## name ## _t* array1, array_ ## name ## _t* array2) \
{ \
    if(array1->capacity <= array1->len + array2->len) \
    { \
        array1->capacity *= 2; \
        \
        if(array1->capacity <= array1->len + array2->len) \
        { \
            array1->capacity += array2->len; \
        } \
        \
        array1->base = (type*) realloc(array1->base, array1->capacity * sizeof(type)); \
    } \
    \
    memcpy(array1->base + array1->len, array2->base, array2->len * sizeof(type)); \
    array1->len += array2->len; \
} \
\
inline void array_ ## name ## _pop(array_ ## name ## _t* array) \
{ \
    assert(array->len > 0); \
    array->len--; \
} \
\
inline void array_ ## name ## _clear(array_ ## name ## _t* array) \
{ \
    array->len = 0; \
}

// this is apparently necessary for C99
#define array_implement_type(name, type) \
    array_ ## name ## _t* array_ ## name ## _create(unsigned int initial_capacity); \
    array_ ## name ## _t* array_ ## name ## _create_from_pointer(type* pointer, unsigned int len); \
    void array_ ## name ## _free(array_ ## name ## _t* array); \
    type array_ ## name ## _get(array_ ## name ## _t* array, unsigned int idx); \
    void array_ ## name ## _set(array_ ## name ## _t* array, unsigned int idx, type item); \
    unsigned int array_ ## name ## _append(array_ ## name ## _t* array, type item); \
    array_ ## name ## _t* array_ ## name ## _copy(array_ ## name ## _t* array); \
    void array_ ## name ## _append_array(array_ ## name ## _t* array1, array_ ## name ## _t* array2); \
    void array_ ## name ## _pop(array_ ## name ## _t* array); \
    type array_ ## name ## _get_new(array_ ## name ## _t* array); \
    void array_ ## name ## _clear(array_ ## name ## _t* array); \
    void array_ ## name ## _append1(array_ ## name ## _t* array, type item); \
    void array_ ## name ## _appendm(array_ ## name ## _t* array, type* item, unsigned int count);

array_define_type(uint8, unsigned char)
array_define_type(sint16, short)
array_define_type(uint64, uint64_t)
array_define_type(uint, unsigned int)

#endif // ARRAY_H_INCLUDED
