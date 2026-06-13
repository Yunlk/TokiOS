#include "isr.h"
#include "shell.h"
#include <stdint.h>

#define VIDEO ((uint8_t*)0xb8000)

static int cursor = 0;  // position in VGA (index, not byte offset)
static char input[256];
static int  input_len = 0;


static const uint8_t scan_ascii[128] = 
{
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,  ' ', /* ... rest 0 */
};

static inline uint8_t inb(uint16_t port) 
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void cursor_set(int pos)
{
    cursor = pos;
}

void keyboard_init()
{
    // nothing to init
}

void keyboard_handler() 
{
    uint8_t sc = inb(0x60);
    if (sc & 0x80) return; // key release, ignore

    if (sc < 128) 
    {
        uint8_t c = scan_ascii[sc];
        if (!c) return;

        if (c == '\b') 
        {
            if (cursor > 0) {
                cursor--;
                VIDEO[cursor * 2] = ' ';
                VIDEO[cursor * 2 + 1] = 0x0F;
            }
            if (input_len > 0) input_len--;
        } 
        else if (c == '\n') 
        {
            input[input_len] = '\0';
            cursor = (cursor / 80 + 1) * 80;
            shell_run(input);
            input_len = 0;
        } 
        else if (input_len < 255)
        {
            input[input_len++] = c;
            VIDEO[cursor * 2] = c;
            VIDEO[cursor * 2 + 1] = 0x0F;
            cursor++;
        }
    }
}

void cursor_write(const char *s)
{
    for (; *s; s++) {
        if (*s == '\n') {
            cursor = (cursor / 80 + 1) * 80;
        } else {
            VIDEO[cursor * 2] = *s;
            VIDEO[cursor * 2 + 1] = 0x0F;
            cursor++;
        }
    }
}
