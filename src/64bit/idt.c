#include <types.h>
#include <idt.h>
#include <layout.h>
#include <log.h>
#include <tty.h>
#include <x86.h>


// Interrupt Descriptor Table
ALIGNED(PAGE_SIZE) idt_desc_t idt[N_IDT_ENTRIES];

// Interrupt Descriptor Table Register
// used to load an IDT
static idtr_t idtr = {0};


// initializes the IDT
void idt_init(void) {

    // fill the IDTR
    idtr.base = (u64)&idt[0];
    idtr.limit = (u16)sizeof(idt_desc_t) * N_IDT_ENTRIES - 1;

    idt_load(&idtr);

    log_info("IDT loaded\n");
}


// sets an ISR in the IDT
void idt_set_desc(u8 vec, u64 isr, u8 attr, u8 ist) {

    idt_desc_t *desc = &idt[vec];

    desc->base_low = isr & 0xffff;
    desc->cs = CS;
    desc->ist = ist;
    desc->attr = attr;
    desc->base_mid = (isr >> 16) & 0xffff;
    desc->base_high = (isr >> 32) & 0xffffffff;
    desc->zero = 0;
}
