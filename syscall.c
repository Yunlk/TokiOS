#include "syscall.h"
#include "isr.h"
#include "shell.h"

void syscall_handler(struct regs *r)
{
    cursor_write("[ok]");
    cursor_write((const char*)r->ebx);
    switch (r->eax)
    {
        case 1:
            cursor_write((const char*)r->ebx);break;    // cout
        default: break;
    }
}