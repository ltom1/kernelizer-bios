#include <types.h>
#include <x86.h>
#include <mbr.h>
#include <tty.h>
#include <log.h>
#include <ata.h>

u16 retabcd(void) {

    return 0xabcd;
}

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


    ata_read(&ata_drives[0], (u8*)0x4000, 0x0000000000000000, 2);


    while (1);
}
