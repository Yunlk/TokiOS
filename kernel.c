#include "gdt.h"
#include "idt.h"
#include "isr.h"

void keyboard_init();

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
    keyboard_init();
    while(1);
    __asm__ volatile ("hlt");
}