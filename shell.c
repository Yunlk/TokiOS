#include "shell.h"
#include "isr.h"

static int strncmp(const char *a, const char *b, int n)
{
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static int strlen(const char *s)
{
    int n = 0;
    while (*s++) n++;
    return n;
}

void shell_run(const char *cmd)
{
    // skip leading spaces
    while (*cmd == ' ') cmd++;
    if (!*cmd) {
        cursor_write("\n");
        return;
    }

    // find end of command name
    const char *name = cmd;
    while (*cmd && *cmd != ' ') cmd++;
    int name_len = cmd - name;

    // skip to arguments
    while (*cmd == ' ') cmd++;
    const char *args = cmd;

    // ---- dispatch ----
    if (name_len == 3 && strncmp(name, "say", 3) == 0) {
        cursor_write(args);
    }
    // silent ignore on unknown command
    cursor_write("\n");
}
