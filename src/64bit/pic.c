#include <types.h>
#include <pic.h>
#include <tty.h>
#include <x86.h>
#include <log.h>


// sends an End Of Interrupt (EOI) signal for an IRQ
void pic_eoi(u8 irq) {
    // also send it to the slave PIC if it came from there
    if (irq >= 8) x86_outb(PIC_SLAVE_CMD, PIC_EOI);

    x86_outb(PIC_MASTER_CMD, PIC_EOI);
}


// initializes the 8259 Programmable Interrupt Controller.
void pic_init(void) {

    // INT 0x20-0x27    -> master PIC's IRQs
    // INT 0x28-0x2f    -> slave PIC's IRQs
    pic_remap(0x20, 0x28);

    log_info("Initialized 8259 PIC\n");
}


// reinitializes the PIC with the provided system interrupt offsets
void pic_remap(u64 off_master, u64 off_slave) {

    // save masks
    u8 mask_master = x86_inb(PIC_MASTER_DATA);
    u8 mask_slave = x86_inb(PIC_SLAVE_DATA);

    // start initialization sequence
    x86_outb(PIC_MASTER_CMD, PIC_INIT | PIC_ICW4);
    x86_io_wait();
    x86_outb(PIC_SLAVE_CMD, PIC_INIT | PIC_ICW4);
    x86_io_wait();

    // set vector offsets
    x86_outb(PIC_MASTER_DATA, off_master);
    x86_io_wait();
    x86_outb(PIC_SLAVE_DATA, off_slave);
    x86_io_wait();

    // master PIC: slave PIC mapped onto PIN 2
    x86_outb(PIC_MASTER_DATA, (1 << 2));
    x86_io_wait();
    // slave PIC: cascade identity
    x86_outb(PIC_SLAVE_DATA, 2);
    x86_io_wait();

    // 8086 mode
    x86_outb(PIC_MASTER_DATA, PIC_8086);
    x86_io_wait();
    x86_outb(PIC_SLAVE_DATA, PIC_8086);
    x86_io_wait();

    // restore masks
    x86_outb(PIC_MASTER_DATA, mask_master);
    x86_io_wait();
    x86_outb(PIC_SLAVE_DATA, mask_slave);
}


// masks/disables an irq
void pic_mask_irq(u8 irq) {

    u16 port;
 
    if(irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    u8 val = x86_inb(port) | (1 << irq);
    x86_outb(port, val);        
}


// unmasks/enables an irq
void pic_unmask_irq(u8 irq) {

    u16 port;
 
    if(irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    u8 val = x86_inb(port) & ~(1 << irq);
    x86_outb(port, val);        
}
