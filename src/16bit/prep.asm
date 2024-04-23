bits    16


global  PREP_START
global  mmap_entries

extern  a20_enable
extern  error_a20
extern  mmap_detect
extern  error_mmap
extern  lm_check
extern  lm_enter
extern  error_lm

section .text


; 16 bit main function
; finishes all tasks, that have to be done in real mode (with BIOS interrupts)
PREP_START:

    call    a20_enable
    cmp     ax, 0
    je      error_a20

    ; get a map of the memory regions 
    mov     di, MMAP_BUFFER 
    call    mmap_detect
    cmp     eax, 0
    je      error_mmap
    mov     dword [mmap_entries], eax 

    ; check for long mode support
    call    lm_check
    cmp     ax, 0
    je      error_lm

    jmp     lm_enter


; location of the memory region list
MMAP_BUFFER         equ 0x1000

mmap_entries:       dd  0x0000
