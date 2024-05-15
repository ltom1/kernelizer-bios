#include <types.h>
#include <log.h>
#include <tty.h>
#include <x86.h>
#include <pci.h>


// scans all PCI devices and initializes them if possible
void pci_scan_all(void) {

    for (u64 bus = 0; bus < N_PCI_BUSES; bus++) {

        for (u64 dev = 0; dev < N_PCI_DEVICES; dev++) {

            u16 vendor_id = pci_cfg_read(bus, dev, 0, PCI_OFF_VENDOR_ID);
            // device does not exist -> continue searching
            if (vendor_id == 0xffff) continue;

            pci_drive_identify(bus, dev, 0);

            u16 header_type = pci_cfg_read(bus, dev, 0, PCI_OFF_HEADER_TYPE);
            // device has multiple functions
            // enumerate the remaining functions
            if ((header_type & 0x80) != 0) {
                for (u64 func = 1; func < N_PCI_FUNCTIONS; func++) {
                    pci_drive_identify(bus, dev, func);
                }
            }
        }
    }
}


// tries to identify/initialize a PCI device
void pci_drive_identify(u16 bus, u8 dev, u8 func) {

    // get the device type
    u16 class = pci_cfg_read(bus, dev, func, PCI_OFF_CLASS_ALL);

    switch (class) {
        case PCI_CLASS_IDE:
            log_info("Found IDE device \n");
            break;
        case PCI_CLASS_ATA:
            log_info("Found ATA device\n");
            break;
        case PCI_CLASS_SATA:
            log_info("Found SATA device\n");
            break;
        case PCI_CLASS_FLOPPY:
            log_info("Found floppy drive\n");
            break;
        case PCI_CLASS_USB:
            log_info("Found USB controller\n");
            break;
        case PCI_CLASS_SD:
            log_info("Found SD controller\n");
            break;
    }
}


// reads information from a PCI device's configuration address space
// use off and mask to specify the location and the size of the requested information
u32 pci_cfg_read(u8 bus, u8 dev, u8 func, u8 off, u32 mask) {

    // create the configuration address
    u32 addr = (u32)(
            (bus << 16) | 
            (dev << 11) |
            (func << 8) | 
            (off & 0xfc) | 
            ((u32)0x80000000) // enable bit
            );

    // send the address
    x86_outd(PCI_CFG_ADDR, addr);

    // extract the correct part of the data
    u32 data = x86_ind(PCI_CFG_DATA);
    data >>= ((off & 2) * 8);
    data &= mask;

    return data;
}
