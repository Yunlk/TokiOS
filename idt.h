#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_entry 
{
    uint16_t base_low;     // Lower 16 bits of the handler function address
    uint16_t sel;          // Kernel segment selector
    uint8_t always0;       // This must always be zero
    uint8_t flags;         // Flags (type and attributes)
    uint16_t base_high;    // Upper 16 bits of the handler function address
} __attribute__((packed));

struct idt_ptr 
{
    uint16_t limit;        // Size of the IDT
    uint32_t base;         // Base address of the IDT
} __attribute__((packed));
void idt_install();
#endif
