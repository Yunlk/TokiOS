ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot : {
        *(.multiboot)
    }

    .multiboot2 : {
        *(.multiboot2)
    }

    .note.Xen : {
        *(.note.Xen)
    }

    .text : {
        *(.text)
        *(.rodata)
        *(.data)
    }

    .bss ALIGN(4096) : {
        *(.bss)
        *(COMMON)
    }
}
