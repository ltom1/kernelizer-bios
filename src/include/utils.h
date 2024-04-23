#pragma once

// utilities for operating on large chunks of memory

#include <types.h>


void mem_set(u8 *dest, u8 val, u32 n_bytes);
void mem_cpy(u8 *dest, u8 *src, u32 n_bytes);
