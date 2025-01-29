#include "heap.h"

#include <stdio.h>

#include <util/util.h>

void *aligned_malloc(size_t size, size_t alignment) {
    void *t_ptr; // temp pointer
    void *a_ptr; // aligned pointer

    for (int i = 0;; i++) {
        t_ptr = kmalloc(alignment * i);
        a_ptr = kmalloc(size);

        if ((size_t)a_ptr % alignment == 0)
            break;

        kfree(a_ptr);
        kfree(t_ptr);
    }

    debugf_debug("ALIGNED %p!\n", a_ptr);

    t_ptr = NULL;

    return a_ptr;
}
