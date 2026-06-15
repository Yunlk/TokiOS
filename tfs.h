#ifndef TFS_H
#define TFS_H
#include <stdint.h>

#define MAX_FILES 16
#define MAX_FNAME 28
#define FS_BASE 0x300000
#define FS_SIZE 0x100000 //1MB  

typedef struct 
{
    char name[MAX_FNAME];
    uint32_t size;
    uint32_t offset;
    uint8_t  used;
} file_t;

void tfs_init(void);
int tfs_create(const char *name, const char *data, uint32_t len);
int tfs_read(const char *name, char *buf, uint32_t bufsize);
int tfs_delete(const char *name);
void tfs_list(void);

#endif