#include "array.h"

// this is apparently necessary for C99
array_t* array_create(unsigned int item_size, unsigned int initial_capacity);
array_t* array_create_from_pointer(void* pointer, unsigned int item_size, unsigned int len);
void array_free(array_t* array);
void* array_get(array_t* array, unsigned int idx);
void array_set(array_t* array, unsigned int idx, void* item);
unsigned int array_append(array_t* array, void* item);
array_t* array_copy(array_t* array);
array_t* array_append_array(array_t* array1, array_t* array2);
