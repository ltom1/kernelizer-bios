bits    16      ; cpu starts in real mode: 16 bit instructions


; export for second part and debugging
global  MBR_START
global  disk_read
global  dap
global  boot_drive

extern  bootmain

section  .mbr   exec    ; see linker.ld


; execution starts here MBR_START:

    ; flush the code segment register cs by doing a far jump
    jmp     0x0000:.flush
    
.flush:

    ; initialize segments
    cli
    mov     ax, 0x0000      ; code loaded at 0x0000:0x7c00 - C will need empty segment registers
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    ; initialize stack
    xor     ax, ax
    mov     ss, ax
    mov     ax, BOOT_STACK
    mov     sp, ax
    mov     bp, ax
    sti
    
    ; save the boot drive number (put in dl by the BIOS)
    mov     byte [boot_drive], dl

    ; load the second part of the bootloader 
    ; the correct values for reading are already present in the dap (see at the bottom)
    call    disk_read

    ; jump to the start of the second part that is already written in C
    jmp     bootmain


; reads sectors from the boot drive according to dap
; edit dap to specify the desired address, location on the disk and size
disk_read:
    
    clc                         ; carry flag: set if an error occurs
    mov     si, dap             ; si needs to hold the address of the disk acces packet
    mov     ah, 0x42            ; int 0x13 + ah=0x42 -> read sectors from disk using lba addresses
    mov     dl, [boot_drive]    ; read from the current boot drive
    int     0x13

    jc      .error              
    ret

; displays an error message and halts
.error:
    mov     al, 0x44    ; display character 'D' for 'D'isk
    mov     ah, 0x0e    ; BIOS int 0x10 + ah=0x0e -> write character in TTY mode
    int     0x10

    ; properly halt the processor (hlt on its own is not enough, neither is a loop)
    cli

    .loop:
        hlt
        jmp .loop


boot_drive:         db      0x00

BOOT_STACK          equ     0x7c00

BOOT_PART_BOOTABLE  equ     0x7c00 + 0x1be
STAGE2_LOAD_ADDR    equ     0x7c00 + 0x200

STAGE2_SIZE_SECS    equ     0x20
STAGE2_START_LBA    equ     0x01


align 4

; structur of the DAP (Disk Access Packet)
; used for reading sectors from a disk using BIOS interrupts
dap:
    db	0x10                ; size 16 bytes
    db	0x00                ; always 0
num_sectors:	
    dw	STAGE2_SIZE_SECS	; will be set to the sectors actually written
buf:	
    dw	STAGE2_LOAD_ADDR	; memory buffer destination address for the rest of the bootloader
    dw	0		            ; segment (unused in this case)
lba:	
    dd	STAGE2_START_LBA    ; low 32 bits of the LBA
    dd	0		            ; high 16 bits of the LBA (unused in this case)
