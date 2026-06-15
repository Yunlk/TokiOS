#ifndef PROC_H
#define PROC_H

#include <stdint.h>

#define USER_CODE_BASE 0x200000
#define USER_STACK_TOP 0x2FFFFC

extern volatile int in_ring3;

void proc_start(uint32_t entry,uint32_t stack_top);
int  proc_load(const char *name);

#endif // PROC_H