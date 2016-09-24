#include "array.h"
#include "bitstream.h"

// this is apparently necessary for C99
array_t* array_create(unsigned int item_size, unsigned int initial_capacity);
array_t* array_create_from_pointer(void* pointer, unsigned int item_size, unsigned int len);
void array_free(array_t* array);
void* array_get(array_t* array, unsigned int idx);
void array_set(array_t* array, unsigned int idx, void* item);
unsigned int array_append(array_t* array, void* item);
array_t* array_copy(array_t* array);
void array_append_array(array_t* array1, array_t* array2);
void array_pop(array_t* array);
void* array_get_new(array_t* array);
void array_clear(array_t* array);
void array_append1(array_t* array, void* item);
void array_appendm(array_t* array, void* item, unsigned int count);

bitstream_t* bitstream_create();
void bitstream_append(bitstream_t* bs, bool value);
bool bitstream_read(bitstream_t* bs);
bool bitstream_pop(bitstream_t* bs);
void bitstream_free(bitstream_t* bs);
void bitstream_array_adjust(bitstream_t* bs);
bool bitstream_peek(bitstream_t* bs);
