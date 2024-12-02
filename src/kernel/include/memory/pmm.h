#ifndef PMM_H
#define PMM_H 1

#include <stddef.h>

#include <memory/freelists/freelist.h>

void pmm_init();

void *fl_alloc(size_t bytes);
void fl_free(void *ptr);

#endif
