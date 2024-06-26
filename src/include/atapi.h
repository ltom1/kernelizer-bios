#pragma once


#include <types.h>
#include <ide.h>


u64 atapi_read_capacity(atapi_t* atapi);
u64 atapi_read(void *self, u8 *dest, u64 lba, u64 n_secs);
