#ifndef VMA_H
#define VMA_H 1

#include "memory/vmm.h"

#include <stdint.h>
#include <stdbool.h>

void* vma_alloc(vmm_context_t* ctx, size_t size, bool map_allocation);
void vma_free(vmm_context_t* ctx, void* ptr, bool unmap_allocation);

#endif