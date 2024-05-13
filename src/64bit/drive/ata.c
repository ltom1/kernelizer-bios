#include <types.h>
#include <x86.h>
#include <log.h>
#include <tty.h>
#include <ata.h>
#include <drive.h>


ata_t ata_drives[N_IDE_DRIVES] = {
    { (drive_t) { DRIVE_NONE, 0, ata_identify, ata_read, 0 }, 
        ATA_PORTS_PRIMARY,     true,   0, 0 },
    { (drive_t) { DRIVE_NONE, 0, ata_identify, ata_read, 0 }, 
        ATA_PORTS_PRIMARY,     false,  0, 0 },
    { (drive_t) { DRIVE_NONE, 0, ata_identify, ata_read, 0 }, 
        ATA_PORTS_SECONDARY,   true,   0, 0 },
    { (drive_t) { DRIVE_NONE, 0, ata_identify, ata_read, 0 }, 
        ATA_PORTS_SECONDARY,   false,  0, 0 },
};


drive_mode_t    cur_mode    = MODE_NONE;
u32             cur_lba     = 0;
u8              cur_drive   = 0;    // port_base + master


// reads sectors from an ATA drive into RAM
// automatically switches between LBA28 and LBA48 to get the optimal performance
u64 ata_read(void *self, u8 *dest, u64 lba, u64 n_secs) {

    ata_t *drive = (ata_t*)self;

    // check if the requested lba is out of bounds
    if ((lba + n_secs) > drive->base.n_secs)
        log_err("ATA read error:\nLBA address exceeds drive size lba=%x size=%x\n", 
                (u64)(lba + n_secs), drive->base.n_secs);
    

    u16 n_secs_read = 0;
    drive_mode_t mode = MODE_NONE;
    u8 cmd = 0;

    // LBA48 is required
    if ((lba > U28_MAX) || (n_secs > U8_MAX)) {
        mode = MODE_LBA48;
        cmd = ATA_CMD_READ48;
    // LBA28 is usually faster
    } else {
        mode = MODE_LBA28;
        cmd = ATA_CMD_READ28;
    }

    ata_select_drive(drive, mode, (u32)lba);

    // clear error port
    x86_outb(ATA_REG_ERROR(drive->port_base), 0);

    // LBA28 does not need the highest few bits
    if (mode == MODE_LBA48) {

        // send sector count high byte
        x86_outb(ATA_REG_SECCOUNT(drive->port_base), (n_secs >> 8) & 0xff);

        // send start lba second half
        x86_outb(ATA_REG_LBA0(drive->port_base), (lba >> 24) & 0xff);
        x86_outb(ATA_REG_LBA1(drive->port_base), (lba >> 32) & 0xff);
        x86_outb(ATA_REG_LBA2(drive->port_base), (lba >> 40) & 0xff);
    }

    // send sector count low byte
    x86_outb(ATA_REG_SECCOUNT(drive->port_base), n_secs & 0xff);

    // send lba first half
    x86_outb(ATA_REG_LBA0(drive->port_base), lba & 0xff);
    x86_outb(ATA_REG_LBA1(drive->port_base), (lba >> 8) & 0xff);
    x86_outb(ATA_REG_LBA2(drive->port_base), (lba >> 16) & 0xff);

    // send READ command
    x86_outb(ATA_REG_CMD(drive->port_base), cmd);

    // read each sector
    while (n_secs_read < n_secs) {

        // wait for drive
        ata_400ns_delay(drive);

        // poll until BSY bit clears or an error occurs
        u8 status = x86_inb(ATA_REG_STATUS(drive->port_base));
        while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
            status = x86_inb(ATA_REG_STATUS(drive->port_base));

        if (status & 0x01) 
            log_err("ATA read error:\nDrive: n_secs=%x (lba=%x n_secs=%x) -> dest=%x\n", 
                    (u64)drive->base.n_secs,
                    (u64)lba, 
                    (u64)n_secs, 
                    (u64)dest);

        // read data into memory (1 sector)
        for (u64 i = 0; i < 256; i++) {
            *(u16*)(dest + n_secs_read * 512 + i * 2) = 
                x86_inw(ATA_REG_DATA(drive->port_base));
        }
        n_secs_read++;
    }
    // return the bytes read
    return n_secs_read * 512;
}


// needed for some ATA operations
void ata_400ns_delay(ata_t *drive) {

    // 4 times reading
    for (u64 i = 0; i < 4; i++)
        x86_inb(ATA_REG_ALTSTATUS(drive->port_base));
}


// identifies information of an ATA drive
void ata_identify(void *self) {

    ata_t *drive = (ata_t*)self;

    ata_select_drive(drive, MODE_IDENTIFY, 0);

    // clear status port
    x86_outb(ATA_REG_ALTSTATUS(drive->port_base), 0);

    // check if there are drives connected to the bus
    if (x86_inb(ATA_REG_STATUS(drive->port_base)) == 0xff) return;

    ata_select_drive(drive, MODE_IDENTIFY, 0);

    // clear registers for IDENTIFY
    x86_outb(ATA_REG_SECCOUNT(drive->port_base), 0);
    x86_outb(ATA_REG_LBA0(drive->port_base), 0);
    x86_outb(ATA_REG_LBA1(drive->port_base), 0);
    x86_outb(ATA_REG_LBA2(drive->port_base), 0);

    // send IDENTIFY command
    x86_outb(ATA_REG_CMD(drive->port_base), ATA_CMD_IDENTIFY);

    // check if drive is connected
    u8 status = x86_inb(ATA_REG_STATUS(drive->port_base));
    if (status == 0x00) return;     // not connected

    // poll until BSY bit clears or an error occurs
    while (((status & 0x80) == 0x80) && ((status & 0x01) != 0x01))
        status = x86_inb(ATA_REG_STATUS(drive->port_base));

    // error
    if (status & 0x01) {

        // check for SATA/ATAPI drive
        u8 lba1 = x86_inb(ATA_REG_LBA1(drive->port_base));
        u8 lba2 = x86_inb(ATA_REG_LBA2(drive->port_base));

        if ((lba1 == ATAPI_LBA1) && (lba2 == ATAPI_LBA2)) {
            drive->base.type = DRIVE_ATAPI;
            return;
        }

        if ((lba1 == SATA_LBA1) && (lba2 == SATA_LBA2)) {
            drive->base.type = DRIVE_SATA;
            return;
        }

        // unknown error
        log_err("Unknown error identifying ATA drive\n")
        return;
    }

    // found an ATA drive
    drive->base.type = DRIVE_ATA;

    // look through drive information
    u16 cur_data;
    for (u64 read = 0; read < 256; read++) {


        cur_data = x86_inw(ATA_REG_DATA(drive->port_base));

        // check if LBA28 is supported
        if (read == 60) {

            // only 1 read -> no temporary variables needed
            drive->n_secs28 = DWORD(
                    x86_inw(ATA_REG_DATA(drive->port_base)),
                    cur_data    // lower half was the last read
                    );
            read++;
        }

        // check if LBA48 is supported
        else if (read == 100) {

            // we need some temporary variables here to preserve the correct word order
            // word0 = cur_data
            u16 word1 = x86_inw(ATA_REG_DATA(drive->port_base));
            u16 word2 = x86_inw(ATA_REG_DATA(drive->port_base));
            u16 word3 = x86_inw(ATA_REG_DATA(drive->port_base));

            drive->n_secs48 = QWORD(
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
    drive->base.n_secs = MAX(drive->n_secs28, drive->n_secs48);
}


// selects an ATA drive by its <index> and a <mode>
// <lba> only necessary for LBA28e
void ata_select_drive(ata_t *drive, drive_mode_t mode, u32 lba) {

    // skip the selecting if the correct settings are already selected
    // port_base and master will be used as an ID for the selected drive
    if ((drive->port_base + drive->master == cur_drive) && (mode == cur_mode)) {
            if (mode != MODE_LBA28) return;     // lba is only relevant for MODE_LBA28
            if (lba == cur_lba) return;  
    }


    u8 data;
    switch (mode) {
        case MODE_IDENTIFY:
            data = drive->master ? ATA_IDENTIFY_SEL_MASTER : ATA_IDENTIFY_SEL_SLAVE;
            break;
        case MODE_LBA28:
            data = drive->master ? ATA_LBA28_SEL_MASTER : ATA_LBA28_SEL_SLAVE;
            data |= (u8)(lba >> 24) & 0x0F;     // has to hold the highest four lba bits
            break;
        case MODE_LBA48:
            data = drive->master ? ATA_LBA28_SEL_MASTER : ATA_LBA48_SELECT_SLAVE;
            break;
        default:
            log_err("Select: invalid selection mode\nmode=%u\n", mode);
    }

    // write the selection value to the drive select register
    x86_outb(ATA_REG_DRIVE_SELECT(drive->port_base), data);

    cur_mode = mode;
    cur_lba = lba;
    cur_drive = drive->port_base + drive->master;
}
