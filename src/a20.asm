bits    16


global  a20_enable

section .text


; enables the a20 address line
; = 21st bit to access more than 1mb of memory
; returns:
; 0 -> failure
; 1 -> success
a20_enable:

        ; first of all
        ; check if the a20 line is already enabled
        call    a20_check
        cmp     ax, 1
        je      .enabled
        
        ; if it isn't...

    ; ...try using 0x15 BIOS interrupt
    .bios:
         
        mov     ax, 0x2403          ; int 0x15: ax=0x2403 -> check if this feature is supported
        int     0x15

        jb      .bios_fail          ; int 0x15 not supported
        cmp     ah, 0
        jnz     .bios_fail          ; int 0x15 not supported


        mov     ax, 0x2402          ; int 0x15: ax=0x2402 -> check a20 gate status
        int     0x15

        jb      .bios_fail          ; couldn't get status
        cmp     ah, 0
        jnz     .bios_fail          ; couldn't get status


        mov     ax, 0x2401          ; int 0x15: ax=0x2402 -> enable a20 gate
        int     0x15

        jb      .bios_fail          ; couldn't enable
        cmp     ah, 0
        jnz     .bios_fail          ; couldn't enable

        ; if there were no errors until now, it should have worked
        call    a20_check            
        cmp     ax, 1
        je      .enabled

    .bios_fail:


    ; ...try using the keyboard controller
    .keyboard:

        cli                         ; disable interrupts

        call    kbd_cmd_wait
        mov     al, 0xad            ; disable keyboard
        out     0x64, al
        
        call    kbd_cmd_wait
        mov     al, 0xd0            ; mode: read 
        out     0x64, al

        call    kbd_data_wait
        in      al, 0x60            ; read value from data port (0x60)
        push    ax                  ; save it for later

        call    kbd_cmd_wait
        mov     al, 0xd1            ; mode: write
        out     0x64, al

        call    kbd_cmd_wait
        pop     ax                  ; restore the read value
        or      al, 2               ; set the second bit
        out     0x60, al            ; send it back

        call    kbd_cmd_wait
        mov     al, 0xae            ; reenable keyboard
        out     0x64, al

        call    kbd_cmd_wait        ; wait until finished
        sti                         ; enable interrupts

        call    a20_check           
        cmp     ax, 1
        je      .enabled
   
        ; todo check in a loop as this method might take some time


    ; ..try enabling it by reading from IO port 0xee
    .port:

        in      al, 0xee            ; the result in al does not matter

        call    a20_check          
        cmp     ax, 1
        je      .enabled

         
    ; ...try using the fast a20 method, which talks directly to the chipset
    ; might be risky, therefore it goes last
    .fast_a20:

        in      al, 0x92
        or      al, 2
        out     0x92, al

        call    a20_check
        cmp     ax, 1
        je      .enabled
        
        ; todo check in a loop as it might take some time


    ; all tries failed...
    .could_not_enable:
        mov     ax, 0               ; return 0
        ret

    ; success
    .enabled:
        mov     ax, 1               ; return 1
        ret


    ; waits for the keyboard controller to be ready for a command
    kbd_cmd_wait:

        in      al, 0x64
        test    al, 2               ; if second bit is not 0, the keyboard controller is busy
        jnz     kbd_cmd_wait
        ret

    ; waits for the keyboard controller to be ready for data
    kbd_data_wait:

        in      al, 0x64
        test    al, 1               ; if first bit is not 0, the keyboard controller is busy
        jnz     kbd_data_wait
        ret


; checks if the a20 address line is enabled
; returns:
; 0 -> disabled
; 1 -> enabled
a20_check:

    ; 1st try: check the boot signature first

        mov     dx, [0x7dfe]        ; save boot signature 0xaa55 at the end of the MBR 
                                    ; 0x7dfe = 0x7c00 + 510 bytes (signature is 2 bytes long)

        mov     bx, 0xffff          
        mov     es, bx

        mov     bx, 0x7e0e          

        
        ; es:bx = ffff:7e0e -> 1 mb after the boot signature 
        ; -> if a20 is not enabled, this sould wrap around and contain the same boot signature
        mov     ax, [es:bx] 

        ; if they don't match, it did not wrap -> a20 line is already enabled
        cmp     ax, dx
        jne     .enabled            

    ; 2nd try: check again but rotate the boot signature (1 byte to the right in this case)

        mov     dx, [0x7dff]        ; the same as above but one byte off (should be 0x00aa)

        mov     bx, 0xffff                
        mov     es, bx

        mov     bx, [0x7e0f]        ; + 1 byte
        mov     ax, [es:bx]

        cmp     ax, dx
        jne     .enabled            


    ; after the second try, if it kept matching, it is pretty sure that the a20 line is disabled
    .disabled:                      
        mov     ax, 0               ; return 0
        ret

    .enabled:                       ; a20 is already enabled
        mov     ax, 1               ; return 1
        ret
