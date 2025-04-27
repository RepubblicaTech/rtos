#include "beap.h"

#include <spinlock.h>

#include <stdio.h>

#include <memory/vma.h>
#include <memory/vmm.h>

#include <util/util.h>

void *beap_alloc_pages(size_t pages) {
    if (!pages) {
        return NULL;
    }

    return vma_alloc(get_current_ctx(), pages, NULL);
}

void beap_free_pages(void *ptr, size_t pages) {
    UNUSED(pages);
    vma_free(get_current_ctx(), ptr, true);
}

lock_t BEAP_LOCK;

void beap_lock() {
    spinlock_acquire(&BEAP_LOCK);
}

void beap_unlock() {
    spinlock_release(&BEAP_LOCK);
}
