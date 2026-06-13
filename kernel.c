#include "gdt.h"
#include "idt.h"
#include "isr.h"

void keyboard_init();

void kernel_main(void)
{
    // Clear screen
    uint16_t *vga = (uint16_t*)0xb8000;
    for (int i = 0; i < 80 * 25; i++)
        vga[i] = (0x0F << 8) | ' ';

    // Print boot message
    const char *msg = "TokiOS booted.";
    for (int i = 0; msg[i]; i++)
        vga[i] = (0x0F << 8) | msg[i];

    gdt_install();
    idt_install();
    keyboard_init();
    cursor_set(80);  // cursor to line 2, after boot message

    for(;;) __asm__ volatile("hlt");
}