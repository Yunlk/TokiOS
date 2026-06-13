.set MAGIC,0x1BADB002
.set FLAGS,0x00000003
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
    .global _start
_start:
    mov $stack_top, %esp
    call kernel_main
    cli

1:  hlt
    jmp 1b

#=====gdt flush=====

.global gdt_flush
gdt_flush:
    movl 4(%esp), %eax
    lgdt (%eax)

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    ljmp $0x08, $flush
flush:
    ret

#=====idt_flush=====

.global idt_flush
idt_flush:
    movl 4(%esp), %eax
    lidt (%eax)
    ret
