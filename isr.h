#ifndef ISR_H
#define ISR_H
#include <stdint.h>

struct regs
{
    uint32_t ds, es, fs, gs;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed));

extern void isr128(void);
void syscall_handler(struct regs *r);

void isrs_install();
void irq_install();

/* keyboard / cursor API */
void keyboard_init();
void cursor_set(int pos);
int  cursor_get(void);
void cursor_write(const char *s);
void cursor_clear(void);
void cursor_backspace(void);
void exception_handler(struct regs *r);
char getch(void);
void keybuf_clear(void);

extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7(),
            irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

#endif // ISR_H
