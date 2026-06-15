#include "gdt.h"
struct gdt_entry gdt[6];
struct gdt_ptr gp;
static tss_t tss;

extern void gdt_flush(uint32_t);
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_install()
{
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                                  // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);                   // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);                   // Data segment
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);                      // Ring 3
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);                      // Ring 3
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x40); // TSS
    // Flush the GDT
    gdt_flush(6 * 8 - 1);
}

void tss_init()
{
    for(uint32_t i = 0;i < sizeof(tss);i++)
        ((uint8_t*)&tss)[i] = 0;
    tss.ss0 = 0x10; // Kernel data segment
    tss.esp0 = 0x9FC00; // Stack top (just below 1MB)
    asm volatile("mov $0x28, %%ax\n""ltr %%ax" ::: "ax"); // Load TSS
}