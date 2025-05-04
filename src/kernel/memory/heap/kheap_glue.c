#include "util/util.h"
#include <memory/heap/kheap.h>
#include <memory/heap/kheap_glue.h>
#include <memory/vmm/vma.h>
#include <memory/vmm/vmm.h>

void *os_alloc_pages(size_t pages) {
    return vma_alloc(get_current_ctx(), pages, NULL);
}

void os_free_pages(void *ptr, size_t pages) {
    UNUSED(pages);
    vma_free(get_current_ctx(), ptr, true);
}

int kheap_init() {
    kmalloc_init(os_alloc_pages, os_free_pages);
    return 0;
}
