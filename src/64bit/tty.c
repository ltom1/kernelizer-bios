#include <tty.h>
#include <types.h>
#include <x86.h>
#include <utils.h>

#include <stdarg.h>


// clears the screen
void tty_clear_screen(void) {

    u8 *vga = (u8*)VGA_ADDR;

    for (u64 i = 0; i < MAX_COLS * MAX_ROWS; i++) {
        vga[2 * i] = ' ';
        vga[2 * i + 1] = WHITE_ON_BLACK;
    }
    tty_set_cursor(0);
}


// initializes the tty display
void tty_init(void) {

    tty_clear_screen();
    tty_enable_cursor();
}


// enables the cursor
void tty_enable_cursor(void) {

    x86_outb(VGA_CTRL_REG, VGA_CURSOR_CMD);
    x86_outb(VGA_DATA_REG, VGA_CURSOR_ON);
}


// disables the cursor
void tty_disable_cursor(void) {

    x86_outb(VGA_CTRL_REG, VGA_CURSOR_CMD);
    x86_outb(VGA_DATA_REG, VGA_CURSOR_OFF);
}


// moves the cursor to <off> characters from the start of the screen
void tty_set_cursor(u64 off) {

    x86_outb(VGA_CTRL_REG, VGA_CURSOR_OFF_HIGH);
    x86_outb(VGA_DATA_REG, HIGH_BYTE(off));

    x86_outb(VGA_CTRL_REG, VGA_CURSOR_OFF_LOW);
    x86_outb(VGA_DATA_REG, LOW_BYTE(off));
}


// returns the cursor's character offset from the start of the screen
u64 tty_get_cursor(void) {

    // programs the data register to show the low byte of the offset 
    // and loads it into the low byte of offset
    x86_outb(VGA_CTRL_REG, VGA_CURSOR_OFF_LOW);
    u64 off = x86_inb(VGA_DATA_REG);

    // programs the data register to show the high byte of the offset 
    // and adds it onto the high byte of <off> (<< shifting 8bits = 1byte)
    x86_outb(VGA_CTRL_REG, VGA_CURSOR_OFF_HIGH);
    off += x86_inb(VGA_DATA_REG) << 8;

    return off;
}


// converts a two-dimensional screen position to an one-dimensional character offset
u64 tty_pos_to_off(u64 col, u64 row) {

    return row * MAX_COLS + col;
}


// returns the character offset to the next new line given a character offset
u64 tty_offset_new_line(u64 off) {

    // get current row
    u64 row = off / MAX_COLS;

    // return offset of 1st (0) column of the next row
    return tty_pos_to_off(0, row + 1);
}


// checks if text reached the end and scrolls the screen if it has
// returns the new character offset
u64 tty_check_scroll(u64 off) {

    // check if scrolling is needed
    if (off >= MAX_ROWS * MAX_COLS) return tty_scroll_down();

    // else character offset stays the same
    return off;
}


// scrolls down by one line
// returns the new character offset
u64 tty_scroll_down(void) {

    u8 *vga = (u8*)VGA_ADDR;

    // copy second to last row to first row
    mem_cpy(
        vga,
        vga + MAX_COLS * 2,
        MAX_COLS * (MAX_ROWS - 1) * 2
        );

    // clear last row
    u64 off_last_row = tty_pos_to_off(0, MAX_ROWS - 1) * 2;
    for (u64 col = 0; col < MAX_COLS; col++) {
        vga[off_last_row + col * 2] = ' ';
        vga[off_last_row + col * 2 + 1] = WHITE_ON_BLACK;
    }

    off_last_row /= 2;
    tty_set_cursor(off_last_row);
    return off_last_row;
}


// prints a character at the current cursor position
void tty_putc(u8 color, char c) {

    u64 off = tty_get_cursor();
    u8 *vga = (u8*)VGA_ADDR;

    if (!color) {
         color = WHITE_ON_BLACK;    // default value
    }

    if (c == '\n') {
         off = tty_offset_new_line(off) - 1;
    } else {
         vga[2 * off] = c;
         vga[2 * off + 1] = color;
    }

    off++;
    off = tty_check_scroll(off);
    tty_set_cursor(off);

#ifdef DEBUG
    // write the character to the qemu console via debug port 0xe9
    x86_outb(DBG_PORT, c);
#endif 
}


// prints a formatted string to the cursor position
// works like printf
void tty_putf(u8 color, const char *fmt, ...) {

    va_list vl;
    va_start(vl, fmt);

    char c;                 // go character by character
    for (u64 i = 0; (c = fmt[i]) != 0; i++) {
        
        if (c != '%') {     // no variables to paste
            tty_putc(color, c);
            continue;
        }

        i++;                // get the character after the '%'
        c = fmt[i];
        switch (c) {        // determine type of the variable
            case 'd':
                tty_putd(color, (u64)va_arg(vl, u64));
                break;
            case 'u':
                tty_putu(color, (s64)va_arg(vl, u64));
                break;
            case 's':
                tty_puts(color, (const char*)va_arg(vl, u64));
                break;
            case 'c':
                tty_putc(color, (char)va_arg(vl, u64));
                break;
            case 'x':
            case 'p':
                tty_putx(color, (u64)va_arg(vl, u64));
                break;
            case '%':       // %% will result in a single % getting printed
                tty_putc(color, '%');
                break;
            default:        // no matching types -> print everything as it is
                tty_putc(color, '%');
                tty_putc(color, c);
                break;
        }
    }

    va_end(vl);
}


// prints an unsigned integer (u64) at the current cursor position in decimal representation
void tty_putu(u8 color, u64 num) {

    // max value: 18 446 744 073 709 551 615 -> 20 digits
    char buf[20];
    s64 i = 0;

    do {
        buf[i++] = num % 10 + '0';  // calculate ascii character
    } while ((num /= 10) != 0);     // next decimal place

    // print in reverse order
    while (--i >= 0)
        tty_putc(color, buf[i]);
}


// prints a signed integer (s64) at the current cursor position in decimal representation
void tty_putd(u8 color, s64 num) {

    if (num < 0) {
        tty_putc(color, '-');
        num *= -1;
    }
    tty_putu(color, num);
}


// prints an unsigned integer (u64) at the cursor position in hexadecimal representation
// no "0x" prefix
void tty_putx(u8 color, u64 hex) {

    u64 tmp;
    // i -> (0 - 15) u64 has 16 hex digits
    // for each digit...
    for (s64 i = 15; i >= 0; i--) {
        tmp = hex;

        // eliminate all other digits
        tmp = tmp >> i * 4;
        tmp &= 0xf;

        // calculate ascii character
        tmp += ((tmp < 10) ? '0' : 'a' - 10);
        tty_putc(color, tmp);
    }
}


// prints a null-terminated string to the current cursor position
void tty_puts(u8 color, const char *str) {

    u64 i = 0;
    char c = str[i];
    while (c != 0) {
        tty_putc(color, c);
        i++;
        c = str[i];
    }
}
