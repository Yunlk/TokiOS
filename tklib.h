#ifndef TKLIB_H
#define TKLIB_H
#include <stdint.h>

#define TK_MAGIC 0x696B6F74  // toki小端序

typedef struct
{
    uint32_t magic;         // 0x696B6F74
    uint32_t entry;         // entry
    uint32_t code_size;     // size of code
    uint32_t flags;         // 0
}__attribute__((packed)) tk_header_t;

static int tk_validate(tk_header_t *hdr)
{
    if(hdr->magic != TK_MAGIC)
        return -1;
    if(hdr->code_size == 0 || hdr->code_size > 0x100000)
        return -2;
    if(hdr->flags != 0)
        return -3;
    return 0;
}

#endif // TKLIB_H
