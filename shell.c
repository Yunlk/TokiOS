#include "shell.h"
#include "isr.h"
#include "tfs.h"

/* ---- helpers ---- */

static int strncmp(const char *a, const char *b, int n)
{
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static int atoi(const char *s)
{
    int n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

void write_int(int n)
{
    char buf[12];
    int  i = 0;
    if (n < 0)  { buf[i++] = '-'; n = -n; }
    if (n == 0)  buf[i++] = '0';
    while (n) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    int s = (buf[0] == '-') ? 1 : 0;
    int e = i - 1;
    while (s < e) { char t = buf[s]; buf[s] = buf[e]; buf[e] = t; s++; e--; }
    buf[i] = 0;
    cursor_write(buf);
}

static void write_hex(uint32_t n)
{
    cursor_write("0x");
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        uint8_t nib = (n >> (i * 4)) & 0xF;
        buf[7-i] = "0123456789ABCDEF"[nib];
    }
    buf[8] = 0;
    cursor_write(buf);
}

/* ---- dispatch ---- */

void shell_run(const char *cmd)
{
    // skip leading spaces
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    // extract command name
    const char *name = cmd;
    while (*cmd && *cmd != ' ') cmd++;
    int name_len = cmd - name;

    // skip to arguments
    while (*cmd == ' ') cmd++;
    const char *args = cmd;

    // ────────── cout ──────────
    if (name_len == 4 && strncmp(name, "cout", 4) == 0) {
        cursor_write(args);
        return;
    }

    // ────────── clear / cls ──────────
    if ((name_len == 5 && strncmp(name, "clear", 5) == 0) ||
        (name_len == 3 && strncmp(name, "cls",   3) == 0)) {
        cursor_clear();
        return;
    }

    // ────────── where / w ──────────
    if ((name_len == 5 && strncmp(name, "where", 5) == 0) ||
        (name_len == 1 && strncmp(name, "w",     1) == 0)) {
        int pos = cursor_get();
        cursor_write("row=");
        write_int(pos / 80);
        cursor_write(", col=");
        write_int(pos % 80);
        return;
    }

    // ────────── go ──────────
    if (name_len == 2 && strncmp(name, "go", 2) == 0) {
        int row = atoi(args);
        if (row < 0)  row = 0;
        if (row > 24) row = 24;
        cursor_set(row * 80);
        return;
    }

    // ────────── up ──────────
    if (name_len == 2 && strncmp(name, "up", 2) == 0) {
        int n = *args ? atoi(args) : 1;
        int pos = cursor_get();
        pos -= n * 80;
        if (pos < 0) pos = 0;
        cursor_set(pos);
        return;
    }

    // ────────── down ──────────
    if (name_len == 4 && strncmp(name, "down", 4) == 0) {
        int n = *args ? atoi(args) : 1;
        int pos = cursor_get();
        pos += n * 80;
        if (pos >= 80 * 25) pos = 80 * 25 - 80;
        cursor_set(pos);
        return;
    }

    // ────────── home ──────────
    if (name_len == 4 && strncmp(name, "home", 4) == 0) {
        cursor_set(0);
        return;
    }

    // ────────── info ──────────
    if (name_len == 4 && strncmp(name, "info", 4) == 0) {
        uint32_t cr0, cr2, cr3;
        __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
        cursor_write("CR0="); write_hex(cr0);
        cursor_write("  CR2="); write_hex(cr2);
        cursor_write("  CR3="); write_hex(cr3);
        return;
    }

    // ────────── help / ? ──────────
    if ((name_len == 4 && strncmp(name, "help", 4) == 0) ||
        (name_len == 1 && strncmp(name, "?",    1) == 0)) {
        cursor_write("cout clear cls where w go up down home help ? shutdown");
        return;
    }

    // ────────── shutdown ──────────
    if (name_len == 8 && strncmp(name, "shutdown", 8) == 0) {
        cursor_write("Goodbye.");
        for(;;) __asm__ volatile("cli; hlt");
    }

    // ────────── ls ──────────
    if (name_len == 2 && strncmp(name, "ls", 2) == 0) {
        tfs_list();
        return;
    }

    // ────────── show / s ──────────
    if ((name_len == 4 && strncmp(name, "show", 4) == 0) ||
        (name_len == 1 && strncmp(name, "s",    1) == 0)) {
        char buf[256];
        int n = tfs_read(args, buf, sizeof(buf));
        if (n < 0)
            cursor_write("no such file");
        else
            cursor_write(buf);
        return;
    }

    // ────────── new / n ──────────
    if ((name_len == 3 && strncmp(name, "new", 3) == 0) ||
        (name_len == 1 && strncmp(name, "n",   1) == 0)) {

        const char *fname = args;
        while (*fname == ' ') fname++;
        if (!*fname) { cursor_write("new <name> <data>"); return; }
        const char *p = fname;
        while (*p && *p != ' ') p++;
        int fnlen = p - fname;
        while (*p == ' ') p++;
        const char *data = p;
        int dlen = 0;
        while (data[dlen]) dlen++;
        if (fnlen >= MAX_FNAME) { cursor_write("name too long"); return; }

        char fn[MAX_FNAME];
        for (int i = 0; i < fnlen && i < MAX_FNAME-1; i++)
            fn[i] = fname[i];
        fn[fnlen] = '\0';
        int r = tfs_create(fn, data, dlen);
        if (r < 0) cursor_write("create failed");
        return;
    }

    // ────────── del ──────────
    if (name_len == 3 && strncmp(name, "del", 3) == 0) {
        int r = tfs_delete(args);
        if (r < 0) cursor_write("no such file");
        return;
    }

    // ────────── ring3 ──────────
    if (name_len == 5 && strncmp(name, "ring3", 5) == 0) {
        cursor_write("jumping to ring3...");
        extern void proc_start(uint32_t entry);
        proc_start(0x200000);
        return;
    }


    
}
