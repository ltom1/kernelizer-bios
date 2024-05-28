#pragma once


#include <types.h>
#include <heap.h>


typedef enum DRIVE_TYPE {
    DRIVE_NONE,
    DRIVE_ATA,
    DRIVE_SATA,
    DRIVE_ATAPI
} drive_type_t;


// this can be used as an abstract base for other drive type
typedef struct DRIVE {
    drive_type_t type;
    u16 size;   // size of the entire struct in bytes
    u64 n_secs;

    // read(void* self, u8* dest, u64 lba, u64 n_sec;
    u64 (*read)(void*, u8*, u64, u64);

} drive_t;


extern heap_t heap_drives;
