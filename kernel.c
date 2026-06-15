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

    // Preload hello.tk: getch→cout→exit
    //   mov eax,2 / int 0x80 / mov eax,1 / mov ebx,msg / int 0x80 / mov eax,9 / int 0x80
            static const char hello_tk[] = {
        0xB8,0x01,0x00,0x00,0x00,  0x66,0x8C,0xC8,             0x66,0x3D,0x1B,0x00,
        0xBB,0x4A,0x00,0x20,0x00,  0x74,0x05,                  0xBB,0x69,0x00,0x20,0x00,
        0xCD,0x80,
        0xB8,0x01,0x00,0x00,0x00,  0xBB,0x8B,0x00,0x20,0x00,  0xCD,0x80,
        0xB8,0x02,0x00,0x00,0x00,  0xCD,0x80,
        0x3C,0x71,                  0x74,0x12,
        0x3C,0x51,                  0x74,0x0E,
        0xB8,0x01,0x00,0x00,0x00,  0xBB,0xB4,0x00,0x20,0x00,  0xCD,0x80,
        0xEB,0xE3,
        0xB8,0x09,0x00,0x00,0x00,  0xCD,0x80,
        'E','n','t','e','r','e','d',' ','R','i','n','g',' ','3',' ',
        '(','C','S','=','0','x','1','B',')',' ','[','O','K',']',10,0,
        'E','n','t','e','r','e','d',' ','R','i','n','g',' ','3',' ',
        '(','C','S','!','=','0','x','1','B',')',' ','[','F','A','I','L',']',10,0,
        'e','n','t','r','y','=','0','x','2','0','0','0','0','0',10,
        'p','r','e','s','s',' ','a','n','y',' ','k','e','y',',',' ','q',' ','t','o',' ',
        'e','x','i','t',10,0,
        'H','e','l','l','o',' ','f','r','o','m',' ','T','o','k','i','O','S',' ',
        'R','i','n','g',' ','3','!',10,0,
    };
    tfs_create("hello.tk", hello_tk, sizeof(hello_tk));

    cursor_write("\nTokiOS> ");

    for(;;) __asm__ volatile("hlt");
}