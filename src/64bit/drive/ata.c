#include <types.h>
#include <x86.h>
#include <log.h>
#include <tty.h>
#include <ata.h>
#include <ide.h>
#include <drive.h>


// reads sectors from an ATA drive into RAM
// automatically switches between LBA28 and LBA48 to get the optimal performance
u64 ata_read(void *self, u8 *dest, u64 lba, u64 n_secs) {

    ata_t *drive = (ata_t*)self;

    // check if the requested lba is out of bounds
    if ((lba + n_secs) > drive->ide.base.n_secs)
        log_err("ATA read error:\nLBA address exceeds drive size lba=%x size=%x\n", 
                (u64)(lba + n_secs), drive->ide.base.n_secs);
    

    u16 n_secs_read = 0;
    ata_drive_sel_t mode = ATA_SEL_NONE;
    u8 cmd = 0;

    // LBA48 is required
    if ((lba > U28_MAX) || (n_secs > U8_MAX)) {
        mode = ATA_SEL_LBA48;
        cmd = ATA_CMD_READ48;
    // LBA28 is usually faster
    } else {
        mode = ATA_SEL_LBA28;
        cmd = ATA_CMD_READ28;
    }

    ide_select_drive(drive->ide.cmd, drive->ide.slave, mode, (u32)lba);

    // clear error port
    x86_outb(ATA_REG_ERROR(drive->ide.cmd), 0);

    // LBA28 does not need the highest few bits
    if (mode == ATA_SEL_LBA48) {

        // send sector count high byte
        x86_outb(ATA_REG_SECCOUNT(drive->ide.cmd), (n_secs >> 8) & 0xff);

        // send start lba second half
        x86_outb(ATA_REG_LBA0(drive->ide.cmd), (lba >> 24) & 0xff);
        x86_outb(ATA_REG_LBA1(drive->ide.cmd), (lba >> 32) & 0xff);
        x86_outb(ATA_REG_LBA2(drive->ide.cmd), (lba >> 40) & 0xff);
    }

    // send sector count low byte
    x86_outb(ATA_REG_SECCOUNT(drive->ide.cmd), n_secs & 0xff);

    // send lba first half
    x86_outb(ATA_REG_LBA0(drive->ide.cmd), lba & 0xff);
    x86_outb(ATA_REG_LBA1(drive->ide.cmd), (lba >> 8) & 0xff);
    x86_outb(ATA_REG_LBA2(drive->ide.cmd), (lba >> 16) & 0xff);

    // send READ command
    x86_outb(ATA_REG_CMD(drive->ide.cmd), cmd);

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
            log_err("ATA read error:\nDrive: n_secs=%x (lba=%x n_secs=%x) -> dest=%x\n", 
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
