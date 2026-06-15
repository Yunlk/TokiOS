#include "tfs.h"
#include "isr.h"

static file_t files[MAX_FILES];
static uint32_t next_free;

void tfs_init()
{
    for (int i = 0; i < MAX_FILES; i++)
        files[i].used = 0;
    next_free = 0;
}

int tfs_create(const char *name,const char *data,uint32_t len)
{
    if (len > FS_SIZE - next_free)
        return -1; // Not enough space

    int slot = -1;
    for(int i = 0;i < MAX_FILES; i++) 
    {
        if (!files[i].used) 
        {
            slot = i;
            break;
        }
    }
    if(slot < 0)
        return -1; // No free slot
    
    for(int i = 0; i < MAX_FNAME && name[i]; i++)
        files[slot].name[i] = name[i];

    char *dest = (char*)(FS_BASE + next_free); // to RAM
    for(uint32_t i = 0; i < len; i++)
        dest[i] = data[i];

    files[slot].size = len;
    files[slot].offset = next_free;
    files[slot].used = 1;
    next_free += len;
    return 0;
}

int tfs_read(const char *name, char *buf, uint32_t bufsize)
{
    for(int i = 0;i < MAX_FILES;i++)
    {
        if(!files[i].used) continue;
        int match  = 1;
        for(int j = 0;j < MAX_FNAME;j++)
        {
            if(files[i].name[j] != name[j])
            {
                match = 0;
                break;
            }
            if(!name[j]) 
                break;
        }
        if(!match)
            continue;

            uint32_t n = files[i].size;
            if(n > bufsize - 1)
                n = bufsize - 1;
            
            char *src = (char*)(FS_BASE + files[i].offset);
            for(uint32_t j = 0; j < n; j++)
                buf[j] = src[j];
            buf[n] = '\0';
            return n;
    }
    return -1; // Not found
}
void tfs_list(void)
{
    cursor_write("name                 size\n");    
    for(int i = 0;i < MAX_FILES;i++)
    {
        if(!files[i].used)
            continue;
        cursor_write(files[i].name);
        cursor_write("                 ");
    }
    
}

int tfs_delete(const char *name)
{
    for(int i = 0;i < MAX_FILES;i++)
    {
        if(!files[i].used) 
            continue;
        int match  = 1;
        for(int j = 0;j < MAX_FNAME;j++)
        {
            if(files[i].name[j] != name[j])
            {
                match = 0;
                break;
            }
            if(!name[j]) 
                break;
        }
        if(match)
        {
            files[i].used = 0;
            return 0;
        }
    }
    return -1; // Not found
}