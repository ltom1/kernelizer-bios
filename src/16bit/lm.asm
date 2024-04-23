bits    16


    %define PAGE_PRESENT    (1 << 0)
    %define PAGE_WRITE      (1 << 1)
    %define PAGE_HUGE       (1 << 7)

global  lm_check
global  lm_enter

extern  bootmain

section .text


; checks if long mode is supported
; returns:
; 0 -> not supported
; 1 -> supported
lm_check:

; first check for cpuid

    ; try to change bit 21 in eflags
    pushfd
    pop     eax
    mov     ecx, eax    ; save current eflags value
    xor     eax, 1 << 21
    push    eax
    popfd
    
    pushfd
    pop     eax
    xor     eax, ecx
    shr     eax, 21     ; isolate the bit
    and     eax, 1      

    push    ecx         ; restore old eflags value
    popfd

    ; check if setting the bit was successful
    test    eax, eax
    jz      .not_supported

; cpuid available
    
    ; check if 0x80000001 function of cpuid is available
    mov     eax, 0x80000000     ; by using the 0x80000000 function
    cpuid
    cmp     eax, 0x80000001
    jb      .not_supported

; 0x80000001 available

    mov     eax, 0x80000001
    cpuid

    ; test for long mode bit (29th bit)
    test    edx, 1 << 29
    jz      .not_supported

; long mode supported 

    mov     ax, 1   ; return 1
    ret

.not_supported:

    mov     ax, 0   ; return 0
    ret


; activates long mode and jumps to 64-bit code
lm_enter:
    
; create the page tables
; 1 gb identity mapping using a single huge 1gb page
; -> 2 page tables in total (PML4, PDPD placed in sequence)

    ; clear page tables using stosd
    mov     edi, PML4_LOCATION
    mov     ecx, 0x800      ; 0x2000 (2 page tables) stosd writes 4 bytes (0x2000 / 4 = 0x800)
    mov     eax, 0x000      ; write zeros 
    cld                     ; write in the right direction (clear direction flag)
    rep     stosd           ; write repeatedly

    ; prepare the PML4
    mov     eax, PDPT_LOCATION
    or      eax, PAGE_PRESENT | PAGE_WRITE
    mov     [PML4_LOCATION], eax

    ; prepare the PDPT
    mov     eax, 0                                      ; identity mapping 0 -> 0
    or      eax, PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE  ; huge 1gb page
    mov     [PDPT_LOCATION], eax

; enter long mode

    ; disable IRQs
    mov     al, 0xff
    out     0xa1, al
    out     0x21, al

    ; wait a bit
    nop
    nop

    ; emptry IDT register -> any unexpected exception will cause a triple fault 
    ; (to avoid undefined behaviour if something goes wrong)
    cli
    lidt    [idt_register]
    sti

    ; enter long mode
    mov     eax, 10100000b          ; PAE and paging bits
    mov     cr4, eax

    mov     edx, PML4_LOCATION      ; cr3 needs to contain the PML4
    mov     cr3, edx

    mov     ecx, 0xc0000080         ; read from the EFER MSR (Model Specific Register)
    rdmsr

    or      eax, 0x00000100         ; write the LME bit (Long Mode Enable)
    wrmsr

    mov     ebx, cr0                ; activate long mode in cr0
    or      ebx,0x80000001          ; enable paging and protection simultaneously
    mov     cr0, ebx

    cli
    lgdt    [gdt.register]           ; load the GDT
    sti

    jmp     CODE_SEG:LM_START       ; load cs with 64 bit segment and flush the instruction cache



bits    64


; starting point of 64-bit code
LM_START:

    ; reload segment registers
    mov     ax, DATA_SEG
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; jump to the bootloader main function written in C
    jmp     bootmain



bits 16

section .data


align 4
idt_register:
    .length       dw 0
    .base         dd 0

 
gdt:
.desc_null:
    dq  0x0000000000000000              ; null descriptor (required)

; see 
; https://wiki.osdev.org/Global_Descriptor_Table for the format
; https://wiki.osdev.org/GDT_Tutorial 64-bit Flat / Long Mode Setup 
.desc_code:
    dq  0x00af9a000000ffff              ; 64-bit code descriptor (exec/read/lm) from 0 to 0xFFFFF
.desc_data:
    dq  0x00ac92000000ffff              ; 64-bit data descriptor (read/write/lm) from 0 to 0xFFFFF
 
 ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary
align   4
    dw  0                               

.register:
    dw  $ - gdt - 1                     ; 16-bit size (limit) of GDT
    dd  gdt                             ; 32-bit base address of GDT
                                        ; (CPU will zero extend to 64-bit)

PML4_LOCATION       equ     0x2000
PDPT_LOCATION       equ     0x3000

; offset of the descriptor in the GDT in bytes
CODE_SEG            equ     0x0008
DATA_SEG            equ     0x0010
