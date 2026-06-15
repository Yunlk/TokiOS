#include "proc.h"
#include "tfs.h"
#include "paging.h"
#include "isr.h"
#include <stdint.h>

#define USER_CODE_BASE 0x200000
#define USER_STACK_TOP 0x2FFFFC

int proc_load(const char *name)
{
    char *buf = (char*)USER_CODE_BASE;
    int   len = tfs_read(name, buf, 0x100000 - USER_CODE_BASE);
    if (len < 0)
        return -1;

    page_map_user(USER_CODE_BASE, USER_CODE_BASE);
    return 0;
}

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
