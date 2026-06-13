ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot : {
        *(.multiboot)
    }

    .multiboot2 : {
        *(.multiboot2)
    }

    .text : {
        *(.text)
        *(.rodata)
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
