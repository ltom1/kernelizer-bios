#include <types.h>
#include <drive.h>
#include <fat32.h>
#include <vfs.h>
#include <tty.h>
#include <log.h>
#include <x86.h>


// initializes a FAT32 filesystem
void fat32_init(void *self) {

    fat32_t *fs = (fat32_t*)self;

    // load the vbr
    fs->base.drive->read(
            fs->base.drive, 
            (u8*)fs->bpb, 
            fs->base.partition->lba_start, 
            1);

    fs->lba_fat = fs->bpb->lba_partition + fs->bpb->n_reserved_secs;
    fs->lba_data = fs->lba_fat + fs->bpb->n_fats * fs->bpb->fat32_secs_per_fat;
    fs->cluster_root = fs->bpb->cluster_root;
    fs->secs_per_fat = fs->bpb->fat32_secs_per_fat;
    fs->secs_per_cluster = fs->bpb->secs_per_cluster;
    fs->cur_fat_loaded = -1;    // none

    // load root directory
    fat32_load_cluster(fs, fs->cache_root, fs->cluster_root);
}


// loads a FAT cluster into memory
void fat32_load_cluster(fat32_t *self, u8 *dest, u32 cluster) {

    self->base.drive->read(
            self->base.drive, 
            dest, 
            self->lba_data + (cluster - 2) * self->secs_per_cluster,  // 2 = first cluster
            self->secs_per_cluster
            );
}


// returns the next cluster in a cluster chain
u32 fat32_next_cluster(fat32_t *self, u32 cur_cluster) {

    u32 fat_cluster = cur_cluster / FAT_ENTRIES_PER_PAGE;
    u32 off_entry = cur_cluster % FAT_ENTRIES_PER_PAGE;

    // cache the fat cluster
    if (fat_cluster != self->cur_fat_loaded) {
            self->base.drive->read(
                self->base.drive, 
                (u8*)self->cache_fat, 
                self->lba_fat + fat_cluster * self->secs_per_cluster,
                self->secs_per_cluster);
    }

    return *(u32*)(self->cache_fat + off_entry * sizeof(u32));
}


// follows and loads a cluster chain for <n_clusters>
// returns the number of clusters that were actually loaded
u64 fat32_load_cluster_chain(fat32_t *self, u8 *dest, u32 start_cluster, u64 n_clusters) {

    u32 cur_cluster = start_cluster;

    for (u64 i = 0; i < n_clusters; i++) {

        fat32_load_cluster(self, dest + i * self->secs_per_cluster * 512, cur_cluster);

        cur_cluster = fat32_next_cluster(self, cur_cluster);
        if (cur_cluster >= FAT32_EOF) return i + 1;
    }
    return n_clusters;
}


// compares a string path to the file name in a FAT32 directory entry
// to the next '/', '\' or '\0'
// path can be shortened ("NAME.EXT" instead of "NAME    EXT"
// returns how many character match (can be used to increment a char pointer)
// -1 -> name does not match
u8 fat32_cmp_path(const char *path_input, const char *path_entry) {

    // compares the file name
    for (u64 i = 0; i < FAT32_NAME; i++) {
        if (path_input[i] == path_entry[i]) continue;

        // limiter encountered
        if ((path_input[i] == '\\') || (path_input[i] == '/') || (path_input[i] == 0)) {
            for (u64 j = i; j < FAT32_NAME + FAT32_EXT; j++) {

                // the rest in the directory entry has to be empty
                if (path_entry[j] != ' ') return -1;
            }
            return i + 1;
        }

        // reached the file extension
        if (path_input[i] == '.') {
            for (u64 j = i; j < FAT32_NAME; j++) {

                // the remaining characters of the name in the directory entry have to be ' '
                if (path_entry[j] != ' ') return -1;
            }

            i++;    // skip the '.'

            // compares the file extension
            for (u64 j = FAT32_NAME; j < FAT32_NAME + FAT32_EXT; ++i, ++j) {
                if (path_input[i] == path_entry[j]) continue;

                // end of input string
                if ((path_input[i] == '\\') || (path_input[i] == '/') || (path_input[i] == 0)) {

                    for (u64 h = j; h < FAT32_NAME + FAT32_EXT; h++) {

                        // rest has to be empty
                        if (path_entry[h] != ' ') return -1;
                    }
                    return i + 1;
                }
                // extension does not match
                return -1;
            }

            return i + 1;
        }
        // name does not match
        return -1;
    }
    // string and directoy entry are exactly the same
    return FAT32_NAME + FAT32_EXT + 1;
}


// loads a file into <buf>
// returns the filesize
u64 fat32_load_file(void *self, const char *path, u8 *buf, u64 n_clusters) {

    fat32_t *fs = (fat32_t*)self;
    fat32_dir_entry_t *cur_entry;
    u8 skip;
    u32 start_cluster = 0;

    path++;     // skip the root '/'
    fat32_dir_entry_t *cur_dir = (fat32_dir_entry_t*)fs->cache_root;
    u32 cur_cluster = fs->cluster_root;

repeat:
    // loop throug all entries in the current directory
    for (u64 i = 0; i < (fs->secs_per_cluster * 512 / sizeof(fat32_dir_entry_t)); i++) {

        cur_entry = (fat32_dir_entry_t*)cur_dir + i;
        skip = fat32_cmp_path(path, cur_entry->name);

        // entry does not match
        if (skip == (u8)-1) continue;

        // entry does match
        cur_cluster = DWORD(cur_entry->cluster_high, cur_entry->cluster_low);

        // entry is a file -> the file we want to load
        if (!(cur_entry->attr & FAT32_DIR)) {
            start_cluster = cur_cluster;
            break;
        }

        // entry is a subdirectory
        // put it into the cache
        fat32_load_cluster(fs, fs->cache_dir, cur_cluster);
        
        // next object of the file path
        path += skip;

        // select the next directory
        cur_dir = (fat32_dir_entry_t*)fs->cache_dir;

        // skip . and .. in subdirectory
        // i will be incremented again by the loop
        i = 1;
    }

    // match found
    // (0 is not a valid cluster as they start with 2)
    if (start_cluster != 0) {
        // load the entire file  
        fat32_load_cluster_chain(fs, buf, start_cluster, n_clusters);

        // return the filesize as an u64 (required for VFS compatibility)
        return (u64)cur_entry->filesize;
    }

    // start_cluster still 0 -> no match found in the directory
    // check the next part of the directory
    cur_cluster = fat32_next_cluster(fs, cur_cluster);
    if (cur_cluster >= FAT32_EOF) log_err("Could not find file %s\n", path);

    // cache the next part
    fat32_load_cluster(fs, fs->cache_dir, cur_cluster);
    cur_dir = (fat32_dir_entry_t*)fs->cache_dir;

    goto repeat;    // start again
}
