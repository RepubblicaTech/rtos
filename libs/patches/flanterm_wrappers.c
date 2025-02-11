#include "flanterm_wrappers.h"

#include <util/util.h>

#include <memory/heap/liballoc.h>

void flanterm_kfree(void *ptr, size_t unused) {
    UNUSED(unused);
    kfree(ptr);
}