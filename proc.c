#include "proc.h"
#include "tfs.h"
#include "paging.h"
#include "isr.h"
#include "shell.h"
#include <stdint.h>
#include "tklib.h"

uint32_t kern_restore_esp;
volatile int in_ring3 = 0;

int proc_load(const char *name)
{
    int size = tfs_get_size(name);
    if (size < (int)sizeof(tk_header_t)) return -1;

    char *hdr_buf = (char*)USER_CODE_BASE;
    // tfs_read 内部有 n > bufsize-1 的截断
    // 传 size+1 补偿，确保 16 字节头部完整读取
    int n = tfs_read(name, hdr_buf, size + 1);
    if (n < 0) return -1;

    // 直接 reinterpret 头部，不拷贝（避免 SSE movdqu #UD）
    tk_header_t *hdr = (tk_header_t *)hdr_buf;
    if (tk_validate(hdr) != 0) return -1;
    if (sizeof(tk_header_t) + hdr->code_size != (uint32_t)size) return -1;

    keybuf_clear();
    page_map_user(USER_CODE_BASE, USER_CODE_BASE);
    page_map_user(USER_STACK_TOP & 0xFFFFF000,
                  USER_STACK_TOP & 0xFFFFF000);

    __asm__ volatile("movl %%ebp, %0" : "=m"(kern_restore_esp));
    in_ring3 = 1;

    // entry 填的是 0x200000，跳过 16 字节头部
    uint32_t final_entry = hdr->entry + sizeof(tk_header_t);

    proc_start(final_entry, USER_STACK_TOP);
    return 0;
}
