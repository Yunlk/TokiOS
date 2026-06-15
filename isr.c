#include "isr.h"

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
    if(r->int_no == 33) // Keyboard IRQ
        keyboard_handler();
}