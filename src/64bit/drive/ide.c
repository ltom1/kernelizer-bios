#include <types.h>
#include <drive.h>
#include <pci.h>
#include <log.h>
#include <tty.h>
#include <x86.h>
#include <ide.h>
#include <ata.h>
#include <heap.h>


// needed for some operations
void ide_400ns_delay(ide_drive_t *drive) {

    // 4 times reading
    for (u64 i = 0; i < 4; i++)
        x86_inb(ATA_REG_ALTSTATUS(drive->ctrl));
}


// scan a single channel of an IDE controller
// detected drives are allocated on <heap_drives>
void ide_scan_channel(port_t cmd, port_t ctrl) {

    bool slave = false;

identify_start:
    ide_select_drive(cmd, slave, ATA_SEL_IDENTIFY, 0);

    // clear status port
    x86_outb(ATA_REG_ALTSTATUS(ctrl), 0);


    // check if there are drives connected to the bus
    if (x86_inb(ATA_REG_STATUS(cmd)) == 0xff) return;

    ide_select_drive(cmd, 0, ATA_SEL_IDENTIFY, 0);
    
    // clear registers for IDENTIFY
    x86_outb(ATA_REG_SECCOUNT(cmd), 0);
    x86_outb(ATA_REG_LBA0(cmd), 0);
    x86_outb(ATA_REG_LBA1(cmd), 0);
    x86_outb(ATA_REG_LBA2(cmd), 0);

    // send IDENTIFY command
    x86_outb(ATA_REG_CMD(cmd), ATA_CMD_IDENTIFY);

    // check if drive is connected
    u8 status = x86_inb(ATA_REG_STATUS(cmd));
    // not connected
    if (status == 0x00) {

        if (slave) return;

        // try the other one
        slave = true;
        goto identify_start;
    }
    
    // poll until BSY bit clears or an error occurs
    while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
        status = x86_inb(ATA_REG_STATUS(cmd));

    // error
    if (status & 0x01) {

        // check for SATA/ATAPI drive
        u8 lba1 = x86_inb(ATA_REG_LBA1(cmd));
        u8 lba2 = x86_inb(ATA_REG_LBA2(cmd));

        // ATAPI drive detected
        if ((lba1 == ATAPI_LBA1) && (lba2 == ATAPI_LBA2)) {

            // allocate a new ATAPI drive
            atapi_t *atapi = heap_alloc(&heap_drives, sizeof(atapi_t));
            atapi->ide.base.type = DRIVE_ATAPI;
            atapi->ide.base.size = sizeof(atapi_t);
            atapi->ide.base.read = 0;
            atapi->ide.base.n_secs = 0;
            atapi->ide.slave = slave;
            atapi->ide.cmd = cmd;
            atapi->ide.ctrl = ctrl;

            log_info("Found ATAPI drive\n");

            return;
        }

        // SATA drive detected
        if ((lba1 == SATA_LBA1) && (lba2 == SATA_LBA2)) {

            // allocate a new SATA drive
            sata_t *sata = heap_alloc(&heap_drives, sizeof(sata_t));
            sata->base.type = DRIVE_SATA;
            sata->base.size = sizeof(sata_t);
            sata->base.read = 0;
            sata->base.n_secs = 0;
            
            log_info("Found SATA drive\n");

            return;
        }

        // unknown error
        log_err("Unknown error identifying ATA drive\n")
    }

    // ATA drive detected
    // allocate a new ATA drive
    ata_t *ata = heap_alloc(&heap_drives, sizeof(ata_t));
    ata->ide.base.type = DRIVE_ATA;
    ata->ide.base.size = sizeof(ata_t);
    ata->ide.base.read = ata_read;
    ata->ide.base.n_secs = 0;            
    ata->ide.slave = slave;
    ata->ide.cmd = cmd;
    ata->ide.ctrl = ctrl;

    log_info("Found ATA drive\n");

    // look through drive information
    u16 cur_data;
    for (u64 read = 0; read < 256; read++) {


        cur_data = x86_inw(ATA_REG_DATA(cmd));

        // check if LBA28 is supported
        if (read == 60) {

            // only 1 read -> no temporary variables needed
            ata->n_secs28 = DWORD(
                    x86_inw(ATA_REG_DATA(cmd)),
                    cur_data    // lower half was the last read
                    );
            read++;
        }

        // check if LBA48 is supported
        else if (read == 100) {

            // we need some temporary variables here to preserve the correct word order
            // word0 = cur_data
            u16 word1 = x86_inw(ATA_REG_DATA(cmd));
            u16 word2 = x86_inw(ATA_REG_DATA(cmd));
            u16 word3 = x86_inw(ATA_REG_DATA(cmd));

            ata->n_secs48 = QWORD(
                DWORD(
                    word3, 
                    word2), 
                DWORD(
                    word1,
                    cur_data)
                );
            read += 3;
        }
    }
    // calculate max size
    ata->ide.base.n_secs = MAX(ata->n_secs28, ata->n_secs48);

    if (slave) return;

    // try identifying the other drive
    slave = true;
    goto identify_start;
}


// selects an ide device and a mode
// <lba> only required for LBA28
void ide_select_drive(port_t cmd, bool slave, ata_drive_sel_t mode, u32 lba) {

    // add (1 << 4) for slave drives
    // add highest 4 lba bits (LBA28)
    u8 data = mode | (slave << 4) | ((u8)(lba >> 24) & 0x0F);

    // write the selection value to the drive select register
    x86_outb(ATA_REG_DRIVE_SELECT(cmd), data);
}


// determines the drives' I/O ports and initializes the device
void ide_initialize(u8 bus, u8 dev, u8 func) {

    u64 prog_if = pci_cfg_read(bus, dev, func, PCI_OFF_PROG_IF);
    port_t cmd1 = IDE_CH1_CMD;
    port_t ctrl1 = IDE_CH1_CTRL;
    port_t cmd2 = IDE_CH2_CMD;
    port_t ctrl2 = IDE_CH2_CTRL;

    // primary channel is in native mode
    if (prog_if & IDE_CH1_MODE_PCI_NATIVE) {
        // update ports
        cmd1 = pci_cfg_read(bus, dev, func, PCI_OFF_BAR0);
        ctrl1 = pci_cfg_read(bus, dev, func, PCI_OFF_BAR1);
    }

    // secondary channel is in native mode
    if (prog_if & IDE_CH2_MODE_PCI_NATIVE) {
        // update ports
        cmd2 = pci_cfg_read(bus, dev, func, PCI_OFF_BAR2);
        ctrl2 = pci_cfg_read(bus, dev, func, PCI_OFF_BAR3);
    }

    // scan both channels
    ide_scan_channel(cmd1, ctrl1);
    ide_scan_channel(cmd2, ctrl2);
}
