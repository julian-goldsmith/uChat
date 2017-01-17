#include "array.h"
#include "bitstream.h"

array_implement_type(uint8, unsigned char)
array_implement_type(sint16, short)
array_implement_type(uint64, uint64_t)
array_implement_type(uint, unsigned int)

bitstream_t* bitstream_create();
void bitstream_append(bitstream_t* bs, bool value);
bool bitstream_read(bitstream_t* bs);
bool bitstream_pop(bitstream_t* bs);
void bitstream_free(bitstream_t* bs);
void bitstream_array_adjust(bitstream_t* bs);
bool bitstream_peek(bitstream_t* bs);

