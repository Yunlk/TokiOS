ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot : {
        *(.multiboot)
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
