#ifndef PROC_H
#define PROC_H

void proc_start(void *entry, void *stack_top);
int  proc_load(const char *name);

#endif // PROC_H