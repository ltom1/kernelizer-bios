#pragma once


#include <types.h>


#define ASM __asm__ __volatile__


// Notes about inline assembly:
//  - ASM("instruction" : "outputs" (destination) : "inputs" (source) : "messed up ressources"
//  - "memory" should prevent gcc from optimizing the instruction away
//  - same thing for static INLINE

typedef u16 port_t;


// reads a byte of data from an I/O port
static INLINE u8 x86_inb(port_t port) {

     u8 res;

     // "=a" (res) -> eax into res
     // "d" (port) -> port into edx
     ASM("in al, dx" : "=a" (res) : "d" (port) : "memory");

     return res;
}


// writes a byte of data to an I/O port
static INLINE void x86_outb(port_t port, u8 data) {

     // "a" (data) -> data into eax
     // "d" (port) -> port into edx
     ASM("out dx, al" : : "a" (data), "d" (port) : "memory");
}


// reads a word of data from an I/O port
static INLINE u16 x86_inw(port_t port) {

     u16 res;

     // "=a" (res) -> eax into res
     // "d" (port) -> port into edx
     ASM("in ax, dx" : "=a" (res) : "d" (port) : "memory");

     return res;
}


// writes a word of data to an I/O port
static INLINE void x86_outw(port_t port, u16 data) {

     // "a" (data) -> data into eax
     // "d" (port) -> port into edx
     ASM("out dx, ax" : : "a" (data), "d" (port) : "memory");
}


// reads a double word of data from an I/O port
static INLINE u32 x86_ind(port_t port) {

     u32 res;

     // "=a" (res) -> eax into res
     // "d" (port) -> port into edx
     ASM("in eax, dx" : "=a" (res) : "d" (port) : "memory");

     return res;
}


// writes a double word of data to an I/O port
static INLINE void x86_outd(port_t port, u32 data) {

     // "a" (data) -> data into eax
     // "d" (port) -> port into edx
     ASM("out dx, eax" : : "a" (data), "d" (port) : "memory");
}


// disables interrupts
static INLINE void x86_cli(void) {
    ASM("cli" : : : "memory");
}


// reenables interrupts
static INLINE void x86_sti(void) {
    ASM("sti" : : : "memory");
}


// halts the processor
// it is not guaranteed to stay paused as there are still interrupts -> use x86_hang instead
static INLINE void x86_hlt(void) {
    ASM("hlt" : : : "memory");
}


// stops the CPU entirely
// nothing will happen any more (no interrupts either)
static INLINE NORETURN void x86_hang(void) {
    x86_cli();
    while (1) x86_hlt();
}


// blocks for a few ms
// useful for i/o operations
static INLINE void x86_io_wait(void) {
    x86_outb(0x80, 0);
}


// writes multiple words of data from a memory address to an I/O port
static INLINE void x86_outsw(port_t port, const void *src, u64 n_words) {

    // clear direction flag
    // repeat <n_words> times the outsw (write from memory to I/O port)
    // + -> input and output at the same time (needs to be modified)
    // "+S" (src)       -> src into rsi
    // "+c" (n_words)   -> n_words into rcx
    // "d" (port)       -> port into rdx
	ASM("cld; rep; outsw"
			: "+S"(src), "+c"(n_words)
			: "d"(port)
            : "memory");
}


// reads multiple words of data from an I/O port into memory
static INLINE void x86_insw(port_t port, const void *dest, u64 n_words) {

    // clear direction flag
    // repeat <n_words> times the insw (read from I/O port into memory)
    // + -> input and output at the same time (needs to be modified)
    // "+D" (dest)      -> dest into rdi
    // "+c" (n_words)   -> n_words into rcx
    // "d" (port)       -> port into rdx
	ASM("cld; rep; insw"
			: "+D"(dest), "+c"(n_words)
			: "d"(port)
            : "memory");
}
