#include "proc.h"
#include "tfs.h"
#include "paging.h"     // page_map_user 
#include "isr.h"
#include <stdint.h>

uint32_t kern_restore_esp;
volatile int in_ring3 = 0;

int proc_load(const char *name)
{
    char *buf = (char*)0x200000;
    int len = tfs_read(name, buf, 0x100000);  // 1MB 
    if (len < 0)
        return -1;

    keybuf_clear();
    page_map_user(0x200000, 0x200000);
    page_map_user(0x2FF000, 0x2FF000);

    __asm__ volatile("movl %%ebp, %0" : "=m"(kern_restore_esp));
    in_ring3 = 1;
    proc_start(0x200000, 0x2FFFFC);

    return 0; 
}

void proc_start(uint32_t entry, uint32_t stack_top)
{
    __asm__ volatile(
        "movw $0x23, %%ax      \n"   
        "movw %%ax, %%ds       \n"
        "movw %%ax, %%es       \n"
        "movw %%ax, %%fs       \n"
        "movw %%ax, %%gs       \n"
        "pushl $0x23           \n"   
        "pushl %[esp]          \n"
        "pushfl                \n"
        "pushl $0x1B           \n"   
        "pushl %[eip]          \n"
        "iret                  \n"
        :
        : [esp] "r"(stack_top), [eip] "r"(entry)
        : "ax"
    );
}


