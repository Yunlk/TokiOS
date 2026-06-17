#include "tfs.h"
#include "isr.h"

static file_t files[MAX_FILES];
static uint32_t next_free;

extern void write_int(int n);

void tfs_init()
{
    for (int i = 0; i < MAX_FILES; i++)
        files[i].used = 0;
    next_free = 0;
}

/* ---- 统一名字查找，返回 slot 或 -1 ---- */
static int tfs_find(const char *name)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) continue;
        int match = 1;
        for (int j = 0; j < MAX_FNAME; j++) {
            if (files[i].name[j] != name[j]) {
                match = 0;
                break;
            }
            if (!name[j]) {
                if (files[i].name[j] != 0) match = 0;
                break;
            }
        }
        if (match) return i;
    }
    return -1;
}

int tfs_create(const char *name, const char *data, uint32_t len)
{
    if (len > FS_SIZE - next_free)
        return -1;

    int slot = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) { slot = i; break; }
    }
    if (slot < 0) return -1;

    for (int i = 0; i < MAX_FNAME && name[i]; i++)
        files[slot].name[i] = name[i];

    char *dest = (char*)(FS_BASE + next_free);
    for (uint32_t i = 0; i < len; i++)
        dest[i] = data[i];

    files[slot].size   = len;
    files[slot].offset = next_free;
    files[slot].used   = 1;
    next_free += len;
    return 0;
}

int tfs_read(const char *name, char *buf, uint32_t bufsize)
{
    int slot = tfs_find(name);
    if (slot < 0) return -1;

    uint32_t n = files[slot].size;
    if (n > bufsize - 1)
        n = bufsize - 1;

    char *src = (char*)(FS_BASE + files[slot].offset);
    for (uint32_t j = 0; j < n; j++)
        buf[j] = src[j];
    buf[n] = '\0';
    return n;
}

void tfs_list(void)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) continue;
        cursor_write("  ");
        cursor_write(files[i].name);
        cursor_write("  ");
        write_int(files[i].size);
        cursor_write("\n");
    }
}

int tfs_delete(const char *name)
{
    int slot = tfs_find(name);
    if (slot < 0) return -1;

    uint32_t gap     = files[slot].size;
    uint32_t gap_off = files[slot].offset;

    /* 后面所有文件的数据向前挪 gap 字节 */
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used || i == slot) continue;
        if (files[i].offset > gap_off) {
            char *src = (char*)(FS_BASE + files[i].offset);
            char *dst = src - gap;
            for (uint32_t j = 0; j < files[i].size; j++)
                dst[j] = src[j];
            files[i].offset -= gap;
        }
    }

    files[slot].used = 0;
    next_free -= gap;
    return 0;
}

int tfs_get_size(const char *name)
{
    int slot = tfs_find(name);
    if (slot < 0) return -1;
    return files[slot].size;
}
