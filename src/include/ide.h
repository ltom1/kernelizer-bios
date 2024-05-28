#pragma once


#include <types.h>
#include <drive.h>
#include <pci.h>
#include <x86.h>


// identify SATA/ATAPI drive
#define SATA_LBA1                   0x3c
#define SATA_LBA2                   0xc3
#define ATAPI_LBA1                  0x14
#define ATAPI_LBA2                  0xeb

// ide device supported/selected modes (PCI prog_if)
#define IDE_CH1_MODE_PCI_NATIVE         (1 << 0)
#define IDE_CH1_MODE_BOTH               (1 << 1)
#define IDE_CH2_MODE_PCI_NATIVE         (1 << 2)
#define IDE_CH2_MODE_BOTH               (1 << 3)
#define IDE_BUSMASTER                   (1 << 7)

// default ide command/control ports (for compatibility mode)
#define IDE_CH1_CMD                     0x1f0
#define IDE_CH1_CTRL                    0x3f6
#define IDE_CH2_CMD                     0x170
#define IDE_CH2_CTRL                    0x376

// IDE register ports
#define ATA_REG_DATA(cmd)               ((cmd) + 0x00)
#define ATA_REG_ERROR(cmd)              ((cmd) + 0x01)
#define ATA_REG_FEATURES(cmd)           ((cmd) + 0x01)
#define ATA_REG_SECCOUNT(cmd)           ((cmd) + 0x02)
#define ATA_REG_LBA0(cmd)               ((cmd) + 0x03)
#define ATA_REG_LBA1(cmd)               ((cmd) + 0x04)
#define ATA_REG_LBA2(cmd)               ((cmd) + 0x05)
#define ATA_REG_DRIVE_SELECT(cmd)       ((cmd) + 0x06)
#define ATA_REG_CMD(cmd)                ((cmd) + 0x07)
#define ATA_REG_STATUS(cmd)             ((cmd) + 0x07)
#define ATA_REG_ALTSTATUS(ctrl)         ((ctrl) + 0x00)
#define ATA_REG_DEVCTRL(ctrl)           ((ctrl) + 0x00)

// ATA commands
#define ATA_CMD_IDENTIFY            0xec
#define ATA_CMD_READ28              0x20
#define ATA_CMD_READ48              0x24

// ATAPI commands
#define ATA_CMD_IDENTIFY_PACKET     0xa1
#define ATA_CMD_PACKET              0xa0


// ATA drive selection values
typedef enum ATA_DRIVE_SEL {
    ATA_SEL_NONE        = 0x00,
    ATA_SEL_IDENTIFY    = 0xa0,
    ATA_SEL_LBA28       = 0xe0,
    ATA_SEL_LBA48       = 0x40
} ata_drive_sel_t;



// there are 2 IDE channels with 2 drives each -> 4 drives
// primary/secondary channel
// master/slave drive
typedef struct IDEDrive {
    drive_t base;
    bool slave;

    // command/control I/O ports
    port_t cmd;
    port_t ctrl;
} ide_drive_t;


typedef struct ATA {
    ide_drive_t ide;

    // size (LBA28/LBA48)
    u32 n_secs28;
    u64 n_secs48;
} ata_t;

typedef struct ATAPI {
    ide_drive_t ide;
} atapi_t;

typedef struct SATA {
    drive_t base;
} sata_t;


void ide_initialize(u8 bus, u8 dev, u8 func);
void ide_400ns_delay(ide_drive_t *drive);
void ide_select_drive(port_t cmd, bool slave, ata_drive_sel_t mode, u32 lba);
void ide_scan_channels(port_t cmd1, port_t ctrl1, port_t cmd2, port_t ctrl2);
drive_type_t ide_drive_identify(ide_drive_t *drive);
