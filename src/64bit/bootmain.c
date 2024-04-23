#include <types.h>
#include <x86.h>
#include <mbr.h>
#include <tty.h>
#include <log.h>


void bootmain(void) {

    // print a welcome message
    tty_clear_screen();
    tty_puts(MIX(GREEN, BLACK), "kernelizer-bios ");
    tty_puts(MIX(GREEN, BLACK), VERSION);
    tty_puts(MIX(GREEN, BLACK), "\n\n");

    log_info("Successfully entered Long Mode.\n");

    while (1);
}
