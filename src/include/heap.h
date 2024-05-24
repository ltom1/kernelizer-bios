#pragma once


#include <types.h>


typedef struct Heap {
    u64 size;
    u64 free;
    u64 top;
    u8 *loc;
} heap_t;

void *heap_alloc(heap_t *self, u64 n_bytes);
