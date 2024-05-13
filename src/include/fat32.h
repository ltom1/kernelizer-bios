#pragma once


#include <types.h>
#include <drive.h>
#include <part.h>
#include <vfs.h>


#define FAT_ENTRIES_PER_PAGE    1024
#define FAT32_EOF               0x0ffffff8
#define FAT32_NAME              8
#define FAT32_EXT               3


#define FAT32_READONLY          0x01
#define FAT32_HIDDEN            0x02
#define FAT32_SYSTEM            0x03
#define FAT32_DIR               0x10
#define FAT32_ARCHIVE           0x20

     
// structure of a BIOS Parameter Block
// contains file system information
typedef struct PACKED BPB {

    u8  jmp[3];
    u8  oem_id[8];
    u16 bytes_per_sector;
    u8  secs_per_cluster;
    u16 n_reserved_secs;
    u8  n_fats;
    u16 n_root_entries;
    u16 fat12_n_secs;
    u8  media_descriptor;

    // IMPORTANT: not used by FAT32
    u16 fat12_secs_per_fat;

    u16 secs_per_track;
    u16 secs_per_head;
    u32 lba_partition;
    u32 fat12_n_secs_large;

    // IMPORTANT: used by FAT32
    u32 fat32_secs_per_fat;

    u16 flags;
    u16 fs_version;
    u32 cluster_root;
    u16 lba_fsinfo;
    u16 lba_backup_bootsector;

    u8  reserved[12];

    u8  drive_num;
    u8  reserved_byte;
    u8  signature;
    u32 volume_id;
    u8  volume_label[11];
    u8  system_id[8];

} bpb_t;


// structure of an entry in a FAT32 directory cluster
typedef struct PACKED FAT32DirEntry {
	char name[8];
	char ext[3];
	u8 attr;
	u8 reserved;
 
    u8 create_100ms; 
	u16 create_time;
	u16 create_date;
	u16 access_date;
	u16 cluster_high;
 
	u16 modified_time;
	u16 modified_date;
	u16 cluster_low;
	u32 filesize;
} fat32_dir_entry_t;


typedef struct FAT32 {

    fs_t base;
    bpb_t *bpb;

    u8 *cache_fat;
    u8 *cache_root;
    u8 *cache_dir;

    // cluster-sized part of the FAT that is currently cached
    u32 cur_fat_loaded;

    u8 secs_per_cluster;
    u32 secs_per_fat;
    u64 lba_data;
    u64 lba_fat;
    u32 cluster_root;

} fat32_t;


void fat32_init(void *self);

void fat32_load_cluster(fat32_t *self, u8 *dest, u32 cluster);
u64 fat32_load_cluster_chain(fat32_t *self, u8 *dest, u32 start_cluster, u64 n_clusters);

u32 fat32_next_cluster(fat32_t *self, u32 cur_cluster);
u8 fat32_cmp_path(const char *path_input, const char *path_entry);

u64 fat32_load_file(void *self, const char *path, u8 *buf, u64 n_clusters);
