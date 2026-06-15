#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>

#define PAGE_PRESENT  (1 << 0)
#define PAGE_RW       (1 << 1)
#define PAGE_USER     (1 << 2)

typedef uint32_t page_dir_entry_t;
typedef uint32_t page_table_entry_t;

void paging_init(void);
void page_map_user(uint32_t vaddr,uint32_t paddr);

#endif
