#include "gdt.h"
#include "paging.h"
#include "idt.h"
#include "isr.h"
#include "tfs.h"

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
    tss_init(); 
    paging_init();
    page_map_user(0x200000, 0x200000); // Map 2-3MB for user space
    idt_install();
    keyboard_init();
    tfs_init();

    // Preload hello.tk: mov eax,1 / mov ebx,msg / int 0x80 / jmp . / msg
    static const char hello_tk[] = {
        0xB8,0x01,0x00,0x00,0x00,
        0xBB,0x0E,0x00,0x20,0x00,
        0xCD,0x80,
        0xEB,0xFE,
        'H','e','l','l','o',' ','f','r','o','m',' ','T','o','k','i','O','S',' ',
        'R','i','n','g',' ','3','!',0
    };
    tfs_create("hello.tk", hello_tk, sizeof(hello_tk));

    cursor_write("\nTokiOS> ");

    for(;;) __asm__ volatile("hlt");
}