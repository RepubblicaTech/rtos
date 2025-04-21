#include "beap.h"

#include <spinlock.h>

#include <stdio.h>

#include <memory/vma.h>
#include <memory/vmm.h>

#include <util/util.h>

unsigned long long page_size = 0x1000; // 4KiB pages

void *beap_alloc(size_t size, size_t *size_out) {
    size_t aligned = ROUND_UP(size, page_size);
    if (size_out)
        size_out[0] = aligned;

    return vma_alloc(get_current_ctx(), aligned / page_size, NULL);
}

void beap_dealloc(void *ptr, size_t pages) {
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

// prints debug info if (HEAP_DEBUG is defined)
#ifdef HEAP_DEBUG
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
