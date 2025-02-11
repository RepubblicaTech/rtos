#include "flanterm_wrappers.h"

#include <util/util.h>

#include <memory/heap/liballoc.h>
#include <memory/paging/paging.h>

void *flanterm_alloc(size_t size) {
    void *ptr = kmalloc(size);

    return ptr;
}

void flanterm_free(void *ptr, size_t size) {
    UNUSED(size);

    kfree(ptr);
}