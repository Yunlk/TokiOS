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

// TSS
typedef struct
{
    uint16_t link, _res0;
    uint32_t esp0;
    uint16_t ss0, _res1;
    uint32_t esp1;
    uint16_t ss1, _res2;
    uint32_t esp2;
    uint16_t ss2, _res3;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, _res4;
    uint16_t cs, _res5;
    uint16_t ss, _res6;
    uint16_t ds, _res7;
    uint16_t fs, _res8;
    uint16_t gs, _res9;
    uint16_t ldtr, _res10;
    uint16_t _res11, iopb;
}__attribute__((packed)) tss_t;
void tss_init(void);
void gdt_install();
#endif // GDH_H