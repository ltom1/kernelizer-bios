#pragma once


#include <types.h>
#include <part.h>


typedef enum DRIVE_TYPE {
    DRIVE_NONE,
    DRIVE_ATA,
    DRIVE_SATA,
    DRIVE_ATAPI
} drive_type_t;


// this can be used as an abstract base for other drive types
typedef struct DRIVE {
    drive_type_t type;
    u64 n_secs;

    // init(void* self)
    void (*init)(void*);
    // read(void* self, u8* dest, u64 lba, u64 n_sec;
    u64 (*read)(void*, u8*, u64, u64);
    // write(void* self, u8* src, u64 lba, u64 n_bytes)
    u64 (*write)(void*, u8*, u64, u64);

} drive_t;
