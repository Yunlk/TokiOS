#ifndef SYSCALL_H
#define SYSCALL_H

#include "isr.h"

#define SYS_COUT     1
#define SYS_GETCH    2
#define SYS_CUR_SET  3
#define SYS_CUR_GET  4
#define SYS_TFS_LIST 5
#define SYS_TFS_READ 6
#define SYS_TFS_CREATE 7
#define SYS_TFS_DELETE 8
#define SYS_EXIT     9
#define SYS_HEX      10
#define SYS_BACKSPACE 11

void syscall_handler(struct regs *r);

#endif // SYSCALL_H