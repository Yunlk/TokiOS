#include "isr.h"
#include "shell.h"
#include "proc.h"
#include <stdint.h>

#define VIDEO ((uint8_t*)0xb8000)

static int cursor = 0;
static char input[256];
static int  input_len = 0;

/* ---- ring buffer for ring3 getch() ---- */
#define KEYBUF_SIZE 32
static volatile char keybuf[KEYBUF_SIZE];
static volatile int  keybuf_read  = 0;
static volatile int  keybuf_write = 0;

void keybuf_clear(void)
{
    keybuf_read  = 0;
    keybuf_write = 0;
}

char getch(void)
{
    while (keybuf_read == keybuf_write)
        __asm__ volatile("sti; hlt; cli");
    char c = keybuf[keybuf_read];
    keybuf_read = (keybuf_read + 1) % KEYBUF_SIZE;
    return c;
}

static const uint8_t scan_ascii[128] =
{
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,  ' ', 0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
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

/* ---- cursor API ---- */

void cursor_set(int pos)  { cursor = pos; }
int  cursor_get(void)     { return cursor; }

void cursor_write(const char *s)
{
    for (; *s; s++) {
        if (*s == '\n') {
            cursor = (cursor / 80 + 1) * 80;
        } else {
            VIDEO[cursor * 2]     = *s;
            VIDEO[cursor * 2 + 1] = 0x0F;
            cursor++;
        }
    }
}

void cursor_clear(void)
{
    for (int i = 0; i < 80 * 25; i++) {
        VIDEO[i * 2]     = ' ';
        VIDEO[i * 2 + 1] = 0x0F;
    }
    cursor = 0;
}

/* ---- keyboard ---- */

void keyboard_init() {}

void keyboard_handler()
{
    uint8_t sc = inb(0x60);
    if (sc & 0x80) return;

    if (sc < 128) {
        uint8_t c = scan_ascii[sc];
        if (!c) return;

        /* push to ring buffer for ring3 getch() */
        int next = (keybuf_write + 1) % KEYBUF_SIZE;
        if (next != keybuf_read) {
            keybuf[keybuf_write] = c;
            keybuf_write = next;
        }

        /* ring3: skip shell input handling */
        if (!in_ring3) {
            if (c == '\b') {
                if (input_len > 0) {
                    cursor--;
                    VIDEO[cursor * 2]     = ' ';
                    VIDEO[cursor * 2 + 1] = 0x0F;
                    input_len--;
                }
            } else if (c == '\n') {
                input[input_len] = '\0';
                cursor_write("\n");
                shell_run(input);
                input_len = 0;
                if (cursor_get() % 80 != 0)
                    cursor_write("\n");
                cursor_write("TokiOS> ");
            } else if (input_len < 255) {
                input[input_len++] = (char)c;
                VIDEO[cursor * 2]     = c;
                VIDEO[cursor * 2 + 1] = 0x0F;
                cursor++;
            }
        }
    }
}
