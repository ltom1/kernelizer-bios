OUTPUT_FORMAT("elf64-x86-64")
load_addr = 0x7c00;

SECTIONS
{
    . = load_addr;
    .mbr            :   {   *(.mbr)  }
    .sign   0x7DFE  :   {   SHORT(0xAA55)   }
    .text           :   {   *(.text)        }
    .data           :   {   *(.data)        }
    .rodata         :   {   *(.rodata)      }
    .bss            :   {   *(.bss)         }
}
