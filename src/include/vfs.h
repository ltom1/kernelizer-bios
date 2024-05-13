#pragma once


#include <part.h>
#include <types.h>
#include <drive.h>


typedef enum FS_TYPE {
    FS_NONE,
    FS_FAT32
} fs_type_t;


typedef struct FS {
    drive_t *drive;
    partition_t *partition;

    // init(void* self, void* info)
    void (*init)(void*);
    // read_file(void* self, const char* path, u8* buf, u64 size) // size might not be fixed
    u64 (*read_file)(void*, const char*, u8*, u64);

} fs_t;
