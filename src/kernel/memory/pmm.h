#ifndef PMM_H
#define PMM_H 1

#include <kernel.h>
#include <stddef.h>

#include "freelists/freelist.h"

extern struct bootloader_data limine_parsed_data;
#define HHDM_OFFSET limine_parsed_data.hhdm_offset

#define PHYS_TO_VIRTUAL(ADDR)  ((uint64_t)ADDR + HHDM_OFFSET)
#define VIRT_TO_PHYSICAL(ADDR) ((uint64_t)ADDR - HHDM_OFFSET)

void pmm_init();

void *fl_alloc(size_t bytes);
void fl_free(void *ptr);

#endif
