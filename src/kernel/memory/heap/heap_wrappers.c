#include "heap.h"

#include <spinlock.h>

#include <stdio.h>

#include <memory/vma.h>
#include <memory/vmm.h>

unsigned long long page_size = 0x1000; // 4KiB pages

void *heap_alloc(size_t size, size_t *size_out) {
    size_t aligned = ROUND_UP(size, page_size);
    size_out[0]    = aligned;

    return vma_alloc(get_current_ctx(), aligned / page_size, NULL);
}

void heap_dealloc(void *ptr) {
    vma_free(get_current_ctx(), ptr);
}

lock_t HEAP_LOCK;

void heap_lock() {
    spinlock_acquire(&HEAP_LOCK);
}

void heap_unlock() {
    spinlock_release(&HEAP_LOCK);
}

// prints debug info if (HEAP_DEBUG is defined)
#ifdef HEAP_DEBUG
void heap_debug(const char *fmt, ...) {
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
