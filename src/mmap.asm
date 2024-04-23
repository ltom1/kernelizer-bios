bits    16

global  mmap_detect

section .text


; detects the memory map
; loads it into buf  
; returns the number of entries 
mmap_detect:

    ; esi will be the entry count
    mov     esi, 0 

    ; this code will make use of the BIOS interrupt 0x15 eax=0xe820
    ; eax   -> 0xe820
    ; ebx   -> continuation ID (initially 0) for the next entry
    ; ecx   -> size of the entry (24)
    ; edx   -> E820_SIGNATURE
    ; es:di -> memory location
    ; afterwards...
    ; eax should contain 0xe820 
    ; ecx should contain how many bytes were actually written
    ; ebx should contain the continuation ID for the next entry
    mov     eax, 0xe820
    mov     ebx, 0x0000
    mov     ecx, MMAP_ENTRY_SIZE
    mov     edx, E820_SIGNATURE

    mov     di, 0x1000
    mov     dword [es:di + 20], 1   ; write a 1 to the last 4 bytes of the entry to support ACPI

    int     0x15

    jc      .failure        ; carry flag on first int call -> unsupported
    test    ebx, ebx        ; if the continuation ID is already 0, there is only one entry... 
    je      .failure        ; which is quite pointless for a list

    ; rest of the checks are somewhere else
    jmp     .checks

.loop_start:
    
    mov     eax, 0xe820             ; was previously overwritten by E820_SIGNATURE
    mov     dword [es:di + 20], 1   ; write a 1 to the last 4 bytes of the entry to support ACPI
    mov     ecx, MMAP_ENTRY_SIZE    ; might have been overwritten too
    mov     edx, E820_SIGNATURE     ; might have been overwritten too

    int     0x15

    jc      .finished               ; carry flag -> list finished

.checks:
    
    cmp     eax, E820_SIGNATURE
    jne     .failure
    cmp     cl, 20                  ; cl <= 20 -> no acpi information
    jbe     .no_acpi

    test    byte [es:di + 20], 1    ; bit 0 set (acpi) -> ignore this entry
    je      .skip_entry

.no_acpi:
    
    ; check if the length field of the entry is 0 -> ignore the entry
    mov     ecx, [es:di + 8]	; low word | high word 
	or      ecx, [es:di + 12]	; is zero only if both are zero -> u64 length is zero
	jz      .skip_entry

	add     di, MMAP_ENTRY_SIZE ; for the next entry
    inc     esi                 ; increase entry count

.skip_entry:

    test    ebx, ebx        ; if the continuation ID is not 0 -> go to the next entry
    jne     .loop_start     

    ; if it is, we have reached the end of the list

.finished:
    mov     eax, esi    ; return the entry count
    ret

.failure:
    mov     eax, 0      ; return 0
    ret



E820_SIGNATURE      equ     0x534d4150
MMAP_ENTRY_SIZE     equ     24
