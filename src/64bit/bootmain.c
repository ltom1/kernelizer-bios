#include <types.h>
#include <x86.h>
#include <mbr.h>
#include <tty.h>
#include <log.h>
#include <ata.h>
#include <vfs.h>
#include <fat32.h>
#include <pci.h>
#include <idt.h>
#include <pic.h>
#include <heap.h>
#include <ata.h>
#include <layout.h>


u8 drives[PAGE_SIZE];
heap_t heap_drives = {
    PAGE_SIZE,
    PAGE_SIZE,
    0,
    drives
};

u8 filesystems[PAGE_SIZE];
heap_t heap_filesystems = {
    PAGE_SIZE,
    PAGE_SIZE,
    0,
    filesystems
};


void bootmain(void) {

    // print a welcome message
    tty_clear_screen();
    tty_puts(MIX(GREEN, BLACK), "kernelizer-bios ");
    tty_puts(MIX(GREEN, BLACK), VERSION);
    tty_puts(MIX(GREEN, BLACK), "\n\n");

    log_info("Successfully entered Long Mode.\n");

   // interrupts
    idt_init();
    pic_init();

    // scan devices
    pci_scan_all();

    while(1);
}
