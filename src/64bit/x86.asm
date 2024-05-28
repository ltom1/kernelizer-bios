bits    64

global  idt_load

section .text


; loads the Interrupt Descriptor Table (Register)
idt_load:
    lidt    [rdi]
    ret
