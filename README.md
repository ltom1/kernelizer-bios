# Kernelizer-BIOS

A 64-bit x86-64 work-in-progress legacy BIOS bootloader.

## Features

- Booting in Legacy BIOS Mode
    - Master Boot Record

- BIOS Interrupts
    - Memory Map Detection
    - Disk Reading (Interrupt 0x13, ah 0x42)

- Flat Long Mode Setup
    - Kernel-only GDT
    - Paging: Identity mapping using huge pages (1gb)

- Drives
    - ATA support (reading only)

- Filesystems
    - FAT32 support (reading only, with subdirectories and no file limit, loading entire files only)

## Testing

You can compile and test the bootloader in QEMU by running the following commands:

Prepare all images
```sh
make init 
```

Compile the bootloader
```sh
make os.img
```

Execute it in QEMU
```sh
make run
```

To debug it, you can use `make debug` (Alacritty Terminal Emulator required) or manually connect GDB. 
