#include "isr.h"
#include "shell.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void irq_handler(struct regs *r)
{
    // Send End of Interrupt (EOI) signal to PICs
    if (r->int_no >= 8) 
        outb(0xA0, 0x20); // Send EOI to slave PIC
    outb(0x20, 0x20); // Send EOI to master PIC

    extern void keyboard_handler();
    if(r->int_no == 1) // Keyboard IRQ
        keyboard_handler();
}

void exception_handler(struct regs *r)
{
    cursor_write("\n!!! EXCEPTION ");
    write_int(r->int_no);
    cursor_write(" !!!\n");

    cursor_write("  EIP:  ");write_int(r->eip);cursor_write("\n");
    cursor_write("  CS:  ");write_int(r->cs);cursor_write("\n");
    cursor_write("  ERR:  ");write_int(r->err_code);cursor_write("\n");
    cursor_write("  EAX:  ");write_int(r->eax);cursor_write("\n");
    cursor_write("  EBX:  ");write_int(r->ebx);cursor_write("\n");
    cursor_write("  ECX:  ");write_int(r->ecx);cursor_write("\n");
    cursor_write("  EDX:  ");write_int(r->edx);cursor_write("\n");
    // page fault
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    cursor_write("  CR2:  ");write_int(cr2);cursor_write("\n");

    cursor_write("\n    [!]TokiOS halted.[!]\n");
    while(1)
    __asm__ volatile("hlt");

}