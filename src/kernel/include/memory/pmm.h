#ifndef PMM_H
#define PMM_H 1

#include <stddef.h>

#include <memory/freelist/freelist.h>

void pmm_init();

freelist_entry **fl_update_entries();

void *kmalloc(size_t bytes);
void kfree(void *ptr);

#endif
