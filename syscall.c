#include "syscall.h"
#include "isr.h"
#include "tfs.h"
#include "proc.h"

extern uint32_t kern_restore_esp;  

void syscall_handler(struct regs *r)
{
    switch (r->eax) {

    case SYS_COUT:
        cursor_write((const char*)r->ebx);
        break;

    case SYS_GETCH: {
        char c = getch();
        r->eax = (uint32_t)c;
        break;
    }


    case SYS_CUR_SET:
        cursor_set(r->ebx);
        break;

    case SYS_CUR_GET:
        r->eax = cursor_get();
        break;

    case SYS_TFS_LIST:
        tfs_list();
        break;

    case SYS_TFS_READ:
        r->eax = tfs_read((const char*)r->ebx, (char*)r->ecx, r->edx);
        break;

    case SYS_TFS_CREATE:
        r->eax = tfs_create((const char*)r->ebx, (const char*)r->ecx, r->edx);
        break;

    case SYS_TFS_DELETE:
        r->eax = tfs_delete((const char*)r->ebx);
        break;

    case SYS_EXIT:
        in_ring3 = 0;
        __asm__ volatile(
            "movl %0, %%esp        \n"
            "movl %%esp, %%ebp     \n"
            "sti                   \n"
            "xorl %%eax, %%eax     \n"
            "leave                 \n"
            "ret                   \n"
            : : "m"(kern_restore_esp)
        );
        __builtin_unreachable();

    case SYS_HEX: {
        char buf[12];
        uint32_t val = r->ebx;
        const char *hex = "0123456789ABCDEF";
        int p = 0;
        buf[p++] = '0';
        buf[p++] = 'x';
        for (int i = 7; i >= 0; i--)
            buf[p++] = hex[(val >> (i * 4)) & 0xF];
        buf[p++] = '\n';
        buf[p] = '\0';
        cursor_write(buf);
        break;
    }

    case SYS_BACKSPACE:
        cursor_backspace();
        break;
    }
}
