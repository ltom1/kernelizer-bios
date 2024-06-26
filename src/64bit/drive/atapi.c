#include <types.h>
#include <ata.h>
#include <atapi.h>
#include <drive.h>
#include <x86.h>
#include <log.h>
#include <tty.h>
#include <ide.h>


// will be set by an IRQ from the ATAPI drive
bool atapi_irq = false;


// returns the capacity of an ATAPI device using the Read Capacity command
u64 atapi_read_capacity(atapi_t* atapi) {

    // note: I have no idea if this works but the result looks fine (format)
    // apart from the fact that it does not match with the filesize of my cdrom image file

    u8 atapi_packet[12] = { ATA_CMD_READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    u8 status;

    // select ATAPI drive
    ide_select_drive(atapi->ide.cmd, atapi->ide.slave, ATA_SEL_NONE, 0);
    ide_400ns_delay(&atapi->ide);

    // send the PACKET command
    x86_outb(ATA_REG_CMD(atapi->ide.cmd), ATA_CMD_PACKET);

    // poll until BSY bit clears or an error occurs
    status = x86_inb(ATA_REG_STATUS(atapi->ide.cmd));
    while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
        status = x86_inb(ATA_REG_STATUS(atapi->ide.cmd));

    // error
    if (status & 0x01) log_err("ATAPI READ CD-ROM CAPACITY\n");

    // send the atapi_packet
    x86_outsw(ATA_REG_DATA(atapi->ide.cmd), atapi_packet, 6);

    // poll until BSY bit clears or an error occurs
    status = x86_inb(ATA_REG_STATUS(atapi->ide.cmd));
    while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
        status = x86_inb(ATA_REG_STATUS(atapi->ide.cmd));

    // error
    if (status & 0x01) log_err("ATAPI READ CD-ROM CAPACITY\n");

    u32 last_lba = DWORD(
            x86_inw(ATA_REG_DATA(atapi->ide.cmd)),
            x86_inw(ATA_REG_DATA(atapi->ide.cmd)));
    
    u32 blocksize = DWORD(
            x86_inw(ATA_REG_DATA(atapi->ide.cmd)),
            x86_inw(ATA_REG_DATA(atapi->ide.cmd)));

    return (last_lba - 1) * blocksize;
}


// reads sectors from an ATAPI drive into RAM
u64 atapi_read(void *self, u8 *dest, u64 lba, u64 n_secs) {

    atapi_t *drive = (atapi_t*)self;
    u8 status;
    u16 n_secs_read = 0;
    
    // bytes 2-5    -> LBA 
    // bytes 6-9    -> transfer length
    u8 atapi_packet[12] = {ATA_CMD_READ, 
                            0,
                            (lba >> 0x18) & 0xff, 
                            (lba >> 0x10) & 0xff, 
                            (lba >> 0x08) & 0xff,
                            (lba >> 0x00) & 0xff,
                            (n_secs >> 0x18) & 0xff, 
                            (n_secs >> 0x10) & 0xff, 
                            (n_secs >> 0x08) & 0xff,
                            (n_secs >> 0x00) & 0xff,
                            0, 0};

    // check if the requested lba is out of bounds
    if ((lba + n_secs) > drive->ide.base.n_secs)
        log_err("ATAPI READ(12):\nLBA address exceeds drive size lba=%x size=%x\n", 
                (u64)(lba + n_secs), drive->ide.base.n_secs);

    // select ATAPI drive
    ide_select_drive(drive->ide.cmd, drive->ide.slave, ATA_SEL_PACKET, 0);
    ide_400ns_delay(&drive->ide);

    // send the PACKET command
    x86_outb(ATA_REG_CMD(drive->ide.cmd), ATA_CMD_PACKET);

    // poll until BSY bit clears or an error occurs
    status = x86_inb(ATA_REG_STATUS(drive->ide.cmd));
    while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
        status = x86_inb(ATA_REG_STATUS(drive->ide.cmd));

    // error
    if (status & 0x01) log_err("ATAPI READ(12)\n");

    // send the atapi_packet
    x86_outsw(ATA_REG_DATA(drive->ide.cmd), atapi_packet, 6);

    // read each sector
    while (n_secs_read < n_secs) {

        // wait for drive
        ide_400ns_delay((ide_drive_t*)drive);

        // poll until BSY bit clears or an error occurs
        u8 status = x86_inb(ATA_REG_STATUS(drive->ide.cmd));
        while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
            status = x86_inb(ATA_REG_STATUS(drive->ide.cmd));

        // error
        if (status & 0x01) 
            log_err("ATAPI READ(12):\nDrive: n_secs=%x (lba=%x n_secs=%x) -> dest=%x\n", 
                    (u64)drive->ide.base.n_secs,
                    (u64)lba, 
                    (u64)n_secs, 
                    (u64)dest);

        // read data into memory (1 sector)
        x86_insw(
                ATA_REG_DATA(drive->ide.cmd), 
                dest + n_secs_read * 512,
                256);
        n_secs_read++;
    }
    // return the bytes read
    return n_secs_read * 512;
}
