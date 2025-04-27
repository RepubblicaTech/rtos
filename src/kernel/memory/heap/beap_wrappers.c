#include "beap.h"

#include <spinlock.h>

#include <stdio.h>

#include <memory/vma.h>
#include <memory/vmm.h>

#include <util/util.h>

lock_t BEAP_LOCK;

void beap_lock() {
    spinlock_acquire(&BEAP_LOCK);
}

void beap_unlock() {
    spinlock_release(&BEAP_LOCK);
}

void *beap_alloc_pages(size_t pages) {
    return vma_alloc(get_current_ctx(), pages, NULL);
}

void beap_free_pages(void *ptr, size_t pages) {
    vma_free(get_current_ctx(), ptr, true);
    UNUSED(pages);
}

// prints debug info if (HEAP_DEBUG is defined)
#ifdef BEAP_DEBUG
void beap_debug(const char *fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length < 0 || length >= (int)sizeof(buffer)) {
        return;
    }

    debugf("[ heap::DEBUG ] %s", buffer);
}
#endif
