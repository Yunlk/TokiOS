#include "paging.h"

static page_dir_entry_t page_directory[1024] __attribute__((aligned(4096)));
static page_table_entry_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_init(void)
{
    for (int i = 0; i < 1024; i++)
        first_page_table[i] = (i * 4096) | PAGE_PRESENT | PAGE_RW;

    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;
    page_directory[0] = (uint32_t)first_page_table | PAGE_PRESENT | PAGE_RW;

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
