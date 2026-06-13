#ifndef ISR_H
#define ISR_H
#include <stdint.h>

struct regs
{
    uint32_t ds, es, fs, gs;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
}__attribute__((packed));

void isrs_install();
void irq_install();
void keyboard_init();
void cursor_set(int pos);
void cursor_write(const char *s);

extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7(),
            irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

#endif // ISR_H