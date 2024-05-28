#pragma once


#include <types.h>
#include <drive.h>
#include <ide.h>


u64 ata_read(void *self, u8 *dest, u64 lba, u64 n_secs);
