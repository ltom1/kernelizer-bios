#pragma once


#include <types.h>


// I/O ports
#define PCI_CFG_ADDR            0xcf8
#define PCI_CFG_DATA            0xcfc

// different header types
#define PCI_HEADER_GENERAL      0x00
#define PCI_HEADER_PCI_PCI      0x01
#define PCI_HEADER_PCI_CARDBUS  0x02

// for reading different parts of the configuration space
#define PCI_OFF_VENDOR_ID       0x00, 0xffff
#define PCI_OFF_DEVICE_ID       0x02, 0xffff
#define PCI_OFF_CMD             0x04, 0xffff
#define PCI_OFF_STATUS          0x06, 0xffff
#define PCI_OFF_REVISION_ID     0x08, 0xff
#define PCI_OFF_PROG_IF         0x09, 0xff
#define PCI_OFF_SUBCLASS        0x0a, 0xff
#define PCI_OFF_CLASS           0x0b, 0xff
#define PCI_OFF_CLASS_ALL       0x0a, 0xffff
#define PCI_OFF_CACHE_LINE_SIZE 0x0c, 0xff
#define PCI_OFF_LATENCY_TIMER   0x0d, 0xff
#define PCI_OFF_HEADER_TYPE     0x0e, 0xff
#define PCI_OFF_BIST            0x0f, 0xff

#define PCI_OFF_BAR0            0x0f, 0xff
#define PCI_OFF_BAR1            0x0f, 0xff
#define PCI_OFF_BAR2            0x0f, 0xff
#define PCI_OFF_BAR3            0x0f, 0xff
#define PCI_OFF_BAR4            0x0f, 0xff
#define PCI_OFF_BAR5            0x0f, 0xff

// device classification
#define PCI_CLASS_IDE           0x0101
#define PCI_CLASS_FLOPPY        0x0102
#define PCI_CLASS_ATA           0x0105
#define PCI_CLASS_SATA          0x0106
#define PCI_CLASS_USB           0x0c03
#define PCI_CLASS_SD            0x0805

#define N_PCI_BUSES             256
#define N_PCI_DEVICES           32
#define N_PCI_FUNCTIONS         8


void pci_scan_all(void);
u32 pci_cfg_read(u8 bus, u8 dev, u8 func, u8 off, u32 mask);

void pci_drive_identify(u16 bus, u8 dev, u8 func);
