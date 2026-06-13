#include "idt.h"
#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void idf_flush(uint32_t);

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
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void idt_install()
{
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    for(int i = 0;i < IDT_ENTRIES; i++)
    {
        uint8_t *entry = (uint8_t*)&idt[i];
        for(int j = 0; j < sizeof(struct idt_entry); j++)
        {
            entry[j] = 0;
        }
    }

    // Remap the PIC
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 0x20); // Master PIC vector offset
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset
    outb(PIC1_DATA, 0x04); // Tell Master PIC about Slave PIC at IRQ2
    outb(PIC2_DATA, 0x02); // Tell Slave PIC its cascade identity
    outb(PIC1_DATA, 0x01); // Set Master PIC to 8086 mode
    outb(PIC2_DATA, 0x01); // Set Slave PIC to 8086 mode

    outb(PIC1_DATA, 0xFF); // Clear Master PIC mask
    outb(PIC2_DATA, 0xFF); // Clear Slave PIC mask

    idf_flush((uint32_t)&idtp);

    asm volatile("sti");
}