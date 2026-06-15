#include "proc.h"
#include <stdint.h>

void proc_start(void *entry, void *stack_top)
{
    uint32_t e = (uint32_t)entry;
    uint32_t s = (uint32_t)stack_top;

    asm volatile(
        "push $0x23\n"
        "push %1\n"
        "pushf\n"
        "push $0x1B\n"
        "push %0\n"
        "iret\n"
        :
        : "r"(e), "r"(s)
    );
}
