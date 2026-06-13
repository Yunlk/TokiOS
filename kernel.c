#include "gdt.h"

void kernel_main(void)
{
    char *video = (char*)0xb8000; // VGA text mode buffer
    const char *msg = "TokiOS booted.";
    for (int i = 0; msg[i]; i++) 
    {
        video[i * 2] = msg[i]; // Character
        video[i * 2 + 1] = 0x0F;
    }
    gdt_install();
    idt_install();
    while(1);
}