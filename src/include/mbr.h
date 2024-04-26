#pragma once


#include <types.h>
#include <part.h>


extern u8 boot_drive;
extern u32 n_mmap_entries;

#define NUM_PARTITIONS  4
partition_t *partitions = (partition_t*)0x7dbe;
