#ifndef ARRAYPOOL_H_INCLUDED
#define ARRAYPOOL_H_INCLUDED

#include <stdint.h>
#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

void array_pool_init();
array_t* array_pool_get();
void array_pool_release(array_t* item);

#ifdef __cplusplus
}
#endif

#endif // ARRAYPOOL_H_INCLUDED
