.set MAGIC,0x1BADB002
.set FLAGS,0x00000003
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

# ── multiboot2 header for QEMU 9+ ──
.section .multiboot2, "ae"
    .align 8
mb2_start:
    .long 0xE85250D6
    .long 0              # arch: i386
    .long mb2_end - mb2_start
    .long -(0xE85250D6 + 0 + (mb2_end - mb2_start))
    .word 0              # end tag type
    .word 0              # flags
    .long 8              # size
mb2_end:

# ── PVH ELF note (QEMU 11 -kernel 唯一认的协议) ──
.section .note.Xen, "a"
    .align 4
    .long 4              # n_namesz = "Xen\0"
    .long 4              # n_descsz = 1 dword
    .long 18             # n_type = XEN_ELFNOTE_PHYS32_ENTRY
    .asciz "Xen"
    .long _start         # entry point

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
#=====IRQ STUBS=====

.macro IRQ num, handler
.global \handler
\handler:
    cli
    pushl $0          
    pushl $\num     
    jmp irq_common_stub
.endm

IRQ 0, irq0
IRQ 1, irq1
IRQ 2, irq2
IRQ 3, irq3
IRQ 4, irq4
IRQ 5, irq5
IRQ 6, irq6
IRQ 7, irq7
IRQ 8, irq8
IRQ 9, irq9
IRQ 10, irq10
IRQ 11, irq11
IRQ 12, irq12
IRQ 13, irq13
IRQ 14, irq14
IRQ 15, irq15

irq_common_stub:
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax    
    pushl %eax
    call irq_handler
    popl %eax

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    addl $8, %esp     
    iret
    
#=====SYSCALL GATE=====

.global isr128
isr128:
    cli
    pushl $0
    pushl $0x80
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax
    pushl %eax
    call syscall_handler
    popl %eax

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    addl $8, %esp
    iret
