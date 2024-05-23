#pragma once


#include <types.h>


// I/O ports
#define PIC_MASTER_CMD	    0x20
#define PIC_MASTER_DATA	    0x21
#define PIC_SLAVE_CMD	    0x10
#define PIC_SLAVE_DATA	    0x11

// commands
#define PIC_EOI		        0x20

// initialization
#define PIC_ICW4	        0x01
#define PIC_INIT	        0x10
#define PIC_8086	        0x01


void pic_init(void);
void pic_remap(u64 off_master, u64 off_slave);
void pic_eoi(u8 irq);
void pic_mask_irq(u8 irq);
void pic_unmask_irq(u8 irq);
