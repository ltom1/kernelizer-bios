#include <types.h>
#include <mbr.h>
#include <x86.h>
#include <a20.h>


// main function of the second part of the bootloader
void bootmain(void) {

    // enable a20 address line
    if (a20_enable() == 0)
        x86_hang();

    


    while(1);
}
