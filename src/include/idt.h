#pragma once


#include <types.h>


#define N_IDT_ENTRIES           256

#define IDT_PRESENT             0x01
#define IDT_INT_GATE            0x8E


// structure of an IDT entry
typedef struct PACKED IDTDescriptor {
    u16 base_low;
    u16 cs;
    u8 ist;
    u8 attr;
    u16 base_mid;
    u32 base_high;
    u32 zero;
} idt_desc_t;

/// structure of the IDT Descriptor
typedef struct PACKED IDTRegister {
    u16 limit;
    u64 base;
} idtr_t;


typedef struct TrapFrame {
    u64 int_vec;
    u64 err_code;
    u64 rip;
    u64 cs;
    u64 flags;
    u64 rsp;
    u64 ds;
} tf_t;



void idt_init(void);
void idt_set_desc(u8 vec, u64 isr, u8 attr, u8 ist);

void idt_load(idtr_t *idtr);
