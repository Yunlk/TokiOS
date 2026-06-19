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

#===== #DF special handler (must not touch stack) =====

.global exc8
exc8:
    cli
    movl $0xB8000, %edi
    movl $0x0F200F44, (%edi)        # "D "
    movl $0x0F210F46, 4(%edi)       # "F!"
    movl $0x0F210F21, 8(%edi)       # "!!"
    movl $0x0F200F20, 12(%edi)      # "  "
1:  hlt
    jmp 1b

#=====EXCEPTION STUBS=====

.macro EXC num, has_err
.global exc\num
exc\num:
    cli
    .if !\has_err
    pushl $0           # 无错误码的异常推 dummy
    .endif
    pushl $\num        # 异常号
    jmp exc_common_stub
.endm

EXC 0,  0
EXC 1,  0
EXC 2,  0
EXC 3,  0
EXC 4,  0
EXC 5,  0
EXC 6,  0
EXC 7,  0
# exc8 (#DF) has its own handler below — must not touch stack
EXC 9,  0
EXC 10, 1
EXC 11, 1
EXC 12, 1
EXC 13, 1
EXC 14, 1
EXC 15, 0
EXC 16, 0
EXC 17, 1
EXC 18, 0
EXC 19, 0
EXC 20, 0
EXC 21, 1
EXC 22, 0
EXC 23, 0
EXC 24, 0
EXC 25, 0
EXC 26, 0
EXC 27, 0
EXC 28, 0
EXC 29, 1
EXC 30, 1
EXC 31, 0

exc_common_stub:
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
    call exception_handler
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

#===== proc_start: pure asm ring0→ring3 =====
# void proc_start(uint32_t entry, uint32_t stack_top)
# C调用: proc_start(entry, stack_top)
# 参数: [ebp+8]=entry, [ebp+12]=stack_top
# 无返回——iret 跳 ring3
.global proc_start
proc_start:
    pushl %ebp
    movl %esp, %ebp
    movl 0x8(%ebp), %eax
    movl 0xc(%ebp), %ebx
    pushl $0x23            # SS
    pushl %ebx             # ESP
    pushfl                 # EFLAGS
    pushl $0x1B            # CS
    pushl %eax             # EIP
    iret
    # never returns
