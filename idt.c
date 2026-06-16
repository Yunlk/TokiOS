#include "idt.h"
#include "isr.h"

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void idt_flush(uint32_t);
extern void isr128(void);


static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void idt_install()
{
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    for(int i = 0;i < IDT_ENTRIES; i++)
        idt[i] = (struct idt_entry){0};

    // Register excetion handlers (0-31)
    extern void exc0(), exc1(), exc2(),	exc3(),	exc4(),	exc5(),	exc6(),	exc7();
    extern void exc8(), exc9(),	exc10(),exc11(),exc12(),exc13(),exc14(),exc15();
    extern void exc16(),exc17(),exc18(),exc19(),exc20(),exc21(),exc22(),exc23();
    extern void exc24(),exc25(),exc26(),exc27(),exc28(),exc29(),exc30(),exc31();

    uint32_t exc_addrs[] = {
    (uint32_t)exc0,  (uint32_t)exc1,  (uint32_t)exc2,  (uint32_t)exc3,
    (uint32_t)exc4,  (uint32_t)exc5,  (uint32_t)exc6,  (uint32_t)exc7,
    (uint32_t)exc8,  (uint32_t)exc9,  (uint32_t)exc10, (uint32_t)exc11,
    (uint32_t)exc12, (uint32_t)exc13, (uint32_t)exc14, (uint32_t)exc15,
    (uint32_t)exc16, (uint32_t)exc17, (uint32_t)exc18, (uint32_t)exc19,
    (uint32_t)exc20, (uint32_t)exc21, (uint32_t)exc22, (uint32_t)exc23,
    (uint32_t)exc24, (uint32_t)exc25, (uint32_t)exc26, (uint32_t)exc27,
    (uint32_t)exc28, (uint32_t)exc29, (uint32_t)exc30, (uint32_t)exc31,
    };

    for(int i = 0;i < 32;i++)
    idt_set_gate(i,exc_addrs[i],0x08,0x8E);
    
    // Remap PIC
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Unmask only IRQ1 (keyboard)
    outb(PIC1_DATA, 0xFD);
    outb(PIC2_DATA, 0xFF);

    // Register IRQ handlers
    extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
    extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    idt_set_gate(0x80, (uint32_t)isr128, 0x08, 0xEF); //trap gate

    idt_flush((uint32_t)&idtp);
    
    __asm__ volatile("sti");
}
