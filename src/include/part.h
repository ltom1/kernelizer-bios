#pragma once


#include <types.h>


typedef struct PACKED Partition {
    u8  attr;
    u8  c_start;
    u8  h_start;
    u8  s_start;
    u8  type;
    u8  c_end;
    u8  h_end;
    u8  s_end;
    u32 lba_start;
    u32 num_sectors;
} partition_t;
