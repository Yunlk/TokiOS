#include "isr.h"
#include "idt.h"

void irq_handler(struct regs *r)
{
    // Send End of Interrupt (EOI) signal to PICs
    if (r->int_no >= 40) 
        outb(0xA0, 0x20); // Send EOI to slave PIC
    outb(0x20, 0x20); // Send EOI to master PIC

    extern void keyboard_handler();
    if(r ->int_no == 33) // Keyboard IRQ
        keyboard_handler();
}