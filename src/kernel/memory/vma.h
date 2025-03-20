#ifndef VMA_H
#define VMA_H 1

#include "memory/vmm.h"

#include <stdbool.h>
#include <stdint.h>

void *vma_alloc(vmm_context_t *ctx, size_t pages, bool map_allocation,
                void *phys);
void vma_free(vmm_context_t *ctx, void *ptr, bool unmap_allocation);

#endif