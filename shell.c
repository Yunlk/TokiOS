#include "shell.h"
#include "isr.h"
#include "tfs.h"
#include "proc.h"
#include <stddef.h>
#include <stdint.h>
/* --------------- write ---------------- */
void write_int(uint32_t n)
{
    char buf[12];
    int i = 0;
    if (n == 0) { cursor_write("0"); return; }
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) {
        char s[2] = { buf[i], 0 };
        cursor_write(s);
    }
}

/* ---------- tiny str helpers ---------- */
static int strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int strlen(const char *s)
{
    int n = 0;
    while (*s++) n++;
    return n;
}

/* ---------- argument parser ---------- */
static int parse_args(const char *input, const char *argv[], int max)
{
    int argc = 0;
    while (*input) {
        while (*input == ' ') input++;
        if (!*input) break;
        if (argc >= max) break;
        argv[argc++] = input;
        while (*input && *input != ' ') input++;
        if (*input) *(char *)input++ = '\0';
    }
    return argc;
}

/* ---------- command declarations ---------- */
static void cmd_ls  (int argc, const char *argv[]);
static void cmd_run (int argc, const char *argv[]);
static void cmd_show(int argc, const char *argv[]);
static void cmd_new (int argc, const char *argv[]);
static void cmd_del (int argc, const char *argv[]);
static void cmd_clear(int argc, const char *argv[]);
static void cmd_info(int argc, const char *argv[]);
static void cmd_help(int argc, const char *argv[]);

/* ---------- command table ---------- */
typedef struct {
    const char *name;
    void (*func)(int argc, const char *argv[]);
    const char *help;
} cmd_t;

static const cmd_t cmd_table[] = {
    {"ls",    cmd_ls,    "list files"},
    {"run",   cmd_run,   "run <file>"},
    {"show",  cmd_show,  "show <file>"},
    {"new",   cmd_new,   "new <file> <data>"},
    {"del",   cmd_del,   "del <file>"},
    {"clear", cmd_clear, "clear screen"},
    {"info",  cmd_info,  "show cpu regs"},
    {"help",  cmd_help,  "show this"},
    {0, 0, 0}
};

/* ---------- shell entry ---------- */
void shell_run(const char *input)
{
    if (!input || !*input) return;

    const char *argv[16];
    int argc = parse_args(input, argv, 16);
    if (argc == 0) return;

    for (int i = 0; cmd_table[i].name; i++) {
        if (strcmp(argv[0], cmd_table[i].name) == 0) {
            cmd_table[i].func(argc, argv);
            return;
        }
    }
    cursor_write("? ");
    cursor_write(argv[0]);
    cursor_write("\n");
}

/* ---------- commands ---------- */

static void cmd_ls(int argc, const char *argv[])
{
    (void)argc; (void)argv;
    tfs_list();
}

static void cmd_run(int argc, const char *argv[])
{
    if (argc < 2) {
        cursor_write("run <file>\n");
        return;
    }
    int ret = proc_load(argv[1]);
    if (ret < 0) {
        cursor_write("file not found: ");
        cursor_write(argv[1]);
        cursor_write("\n");
    }
}

static void cmd_show(int argc, const char *argv[])
{
    if (argc < 2) {
        cursor_write("show <file>\n");
        return;
    }
    char buf[256];
    int len = tfs_read(argv[1], buf, sizeof(buf) - 1);
    if (len < 0) {
        cursor_write("file not found: ");
        cursor_write(argv[1]);
        cursor_write("\n");
        return;
    }
    buf[len] = 0;
    cursor_write(buf);
    cursor_write("\n");
}

static void cmd_new(int argc, const char *argv[])
{
    if (argc < 3) {
        cursor_write("new <file> <data>\n");
        return;
    }
    int ret = tfs_create(argv[1], argv[2], strlen(argv[2]));
    if (ret < 0)
        cursor_write("create failed (maybe full)\n");
    else
        cursor_write("ok\n");
}

static void cmd_del(int argc, const char *argv[])
{
    if (argc < 2) {
        cursor_write("del <file>\n");
        return;
    }
    int ret = tfs_delete(argv[1]);
    if (ret < 0)
        cursor_write("delete failed\n");
    else
        cursor_write("ok\n");
}

static void cmd_clear(int argc, const char *argv[])
{
    (void)argc; (void)argv;
    cursor_clear();
    cursor_write("TokiOS> ");
}

static void cmd_info(int argc, const char *argv[])
{
    (void)argc; (void)argv;
    uint32_t cr0, cr2, cr3;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    cursor_write("CR0: "); write_int(cr0); cursor_write("\n");
    cursor_write("CR2: "); write_int(cr2); cursor_write("\n");
    cursor_write("CR3: "); write_int(cr3); cursor_write("\n");
}

static void cmd_help(int argc, const char *argv[])
{
    (void)argc; (void)argv;
    for (int i = 0; cmd_table[i].name; i++) {
        cursor_write("  ");
        cursor_write(cmd_table[i].name);
        for (int j = strlen(cmd_table[i].name); j < 10; j++)
            cursor_write(" ");
        cursor_write(cmd_table[i].help);
        cursor_write("\n");
    }
}
