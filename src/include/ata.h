#pragma once


#include <types.h>
#include <drive.h>


#define N_IDE_DRIVES                4

// special values in the given register -> SATA/ATAPI drive
#define SATA_LBA1                   0x3c
#define SATA_LBA2                   0xc3
#define ATAPI_LBA1                  0x14
#define ATAPI_LBA2                  0xEB

// port base
#define ATA_PORTS_PRIMARY           0x1F0
#define ATA_PORTS_SECONDARY         0x170

// register ports
#define ATA_REG_DATA(base)          ((base) + 0x00)
#define ATA_REG_ERROR(base)         ((base) + 0x01)
#define ATA_REG_FEATURES(base)      ((base) + 0x01)
#define ATA_REG_SECCOUNT(base)      ((base) + 0x02)
#define ATA_REG_LBA0(base)          ((base) + 0x03)
#define ATA_REG_LBA1(base)          ((base) + 0x04)
#define ATA_REG_LBA2(base)          ((base) + 0x05)
#define ATA_REG_DRIVE_SELECT(base)  ((base) + 0x06)
#define ATA_REG_CMD(base)           ((base) + 0x07)
#define ATA_REG_STATUS(base)        ((base) + 0x07)
#define ATA_REG_ALTSTATUS(base)     ((base) + 0x206)
#define ATA_REG_DEVCTRL(base)       ((base) + 0x206)

// values for selecting drives (different for each mode/command)
#define ATA_IDENTIFY_SEL_MASTER     0xA0
#define ATA_IDENTIFY_SEL_SLAVE      0xB0
#define ATA_LBA28_SEL_MASTER        0xE0
#define ATA_LBA28_SEL_SLAVE         0xF0
#define ATA_LBA48_SELECT_MASTER     0x40
#define ATA_LBA48_SELECT_SLAVE      0x50

// commands
#define ATA_CMD_IDENTIFY            0xEC
#define ATA_CMD_READ28              0x20
#define ATA_CMD_WRITE28             0x30
#define ATA_CMD_READ48              0x24
#define ATA_CMD_WRITE48             0x34
#define ATA_CMD_FLUSH               0xE7


typedef enum DRIVE_MODE {
    MODE_NONE,
    MODE_IDENTIFY,
    MODE_LBA28,
    MODE_LBA48
} drive_mode_t;



// there are 2 ATA buses with 2 drives each -> 4 drives
// primary/secondary bus
// master/slave drive
typedef struct ATA {
    drive_t base;

    // primary/secondary    ATA_PORTS_PRIMARY/ATA_PORTS_SECONDARY
    u16 port_base;
    // master/slave
    bool master;

    // size
    u32 n_secs28;
    u64 n_secs48;
} ata_t;

extern ata_t ata_drives[N_IDE_DRIVES];

void ata_identify(void *self);
void ata_400ns_delay(ata_t *drive);
void ata_select_drive(ata_t *drive, drive_mode_t mode, u32 lba);
u64 ata_read(void *self, u8 *dest, u64 lba, u64 n_secs);
