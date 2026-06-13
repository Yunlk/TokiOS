#ifndef GDH_H
#define GDH_H

#include <stdint.h>
struct gdt_entry 
{
    uint16_t limit_low;     // Lower 16 bits of the limit
    uint16_t base_low;      // Lower 16 bits of the base
    uint8_t base_middle;    // Next 8 bits of the base
    uint8_t access;         // Access flags
    uint8_t granularity;    // Granularity and upper 4 bits of the limit
    uint8_t base_high;     // Last 8 bits of the base
} __attribute__((packed));

struct gdt_ptr 
{
    uint16_t limit;        // Size of the GDT
    uint32_t base;         // Base address of the GDT
} __attribute__((packed));

void gdt_install();
#endif // GDH_H