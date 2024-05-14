#include <types.h>
#include <x86.h>
#include <mbr.h>
#include <tty.h>
#include <log.h>
#include <ata.h>
#include <vfs.h>
#include <fat32.h>


void bootmain(void) {

    // print a welcome message
    tty_clear_screen();
    tty_puts(MIX(GREEN, BLACK), "kernelizer-bios ");
    tty_puts(MIX(GREEN, BLACK), VERSION);
    tty_puts(MIX(GREEN, BLACK), "\n\n");

    log_info("Successfully entered Long Mode.\n");

    // identify all ATA devices connected with IDE
    for (u8 i = 0; i < N_IDE_DRIVES; i++)
        ata_identify(&ata_drives[i]);


    fat32_t fat32 = { 
        (fs_t) { 
            &ata_drives[0].base, 
            partitions, 
            fat32_init,
            fat32_load_file
        }, 
        (bpb_t*)0x7000, 
        (u8*)0x4000,
        (u8*)0x5000,
        (u8*)0x6000
    };

    fat32.base.init(&fat32);
    fat32.base.read_file(&fat32, "/BOOT/BOOT.CFG", (u8*)0x100000, 100);


    while (1);
}
