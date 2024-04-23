# programs
AS=nasm
CC=gcc
LK=ld
DBG=gdb
VM=qemu-system-x86_64
DD=dd
FDISK=fdisk

FDISK_COMMANDS=fdisk_commands.txt

OBJCPY=objcopy
TERM=alacritty
WORKING_DIR=$(shell pwd)

BOOT_ASM_SRC=$(shell find . -type f -name '*.asm')
BOOT_ASM_OBJ=$(patsubst %.asm,%.o,$(BOOT_ASM_SRC))

BOOT_C_SRC=$(shell find . -type f -name '*.c')
BOOT_C_OBJ=$(patsubst %.c,%.o,$(BOOT_C_SRC))

BOOT_HEADERS=$(shell find src/ -type f -name '*.h')

LINKER_SCRIPT=src/linker.ld


IMG=os.img
IMG_SIZE_SEC=6000000

BOOT_SIZE_SEC=32


default: clean run


run: $(IMG)
	$(VM) -d int,cpu_reset,guest_errors,page -no-reboot -debugcon stdio -hda $<

# IMPORTANT: gdb is not made for 16-bit real mode
# local variables will not be correct in gdb (they work fine in qemu though)
debug: $(IMG)
	$(TERM) --working-directory $(WORKING_DIR) -e $(VM) -s -S -d int,cpu_reset,guest_errors,page -no-reboot -debugcon stdio -hda $< &
	$(DBG) BOOT.ELF \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout regs' \
		-ex 'set disassembly-flavor intel' \
		-ex 'break MBR_START' \
		-ex 'break PREP_START' \
		-ex 'break LM_START' \
		-ex 'break bootmain' \
        -ex 'continue'

$(IMG): BOOT.BIN
	# save the partition table of the image
	# it would simply be overwritten by BOOT.BIN otherwise
	$(DD) conv=notrunc if=$@ of=partition_table.mem bs=1 count=70 skip=440
	# write BOOT.BIN
	$(DD) conv=notrunc if=$< of=$@ bs=512 count=$(BOOT_SIZE_SEC)
	# put back the partition table
	$(DD) conv=notrunc if=partition_table.mem of=$@ bs=1 count=70 seek=440


BOOT.BIN: BOOT.ELF
	$(OBJCPY) -O binary $< $@

BOOT.ELF: $(BOOT_HEADERS) $(BOOT_ASM_OBJ) $(BOOT_C_OBJ) $(LINKER_SCRIPT)
	$(LK) -m elf_x86_64 -o $@ -T $(LINKER_SCRIPT) $(BOOT_ASM_OBJ) $(BOOT_C_OBJ)


%.o: %.asm
	$(AS) -g3 -F dwarf -f elf64 $< -o $@

%.o: %.c
	$(CC) -Wall -Isrc/include -masm=intel -mcmodel=large -mno-red-zone -ffreestanding -fno-pie -fno-stack-protector -g -c $< -o $@


# create the image file
img: 
	$(DD) if=/dev/zero of=$(IMG) bs=512 count=$(IMG_SIZE_SEC)
	# FDISK_COMMANDS contains the commands for the partitioning
	$(FDISK) $(IMG) < $(FDISK_COMMANDS)


clean: 
	rm -f -- *.BIN
	rm -f -- *.ELF
	rm -f -- *.mem
	rm -f -- *.o
	rm -f -- */*.o
	rm -f -- */*/*.o
	rm -f -- */*/*/*.o
	rm -f -- */*/*/*/*.o
