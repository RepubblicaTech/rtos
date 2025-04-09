#include "heap.h"

heapmem_node_t *root_node = NULL;

heapmem_node_t *heapnode_init(size_t size) {
    heapmem_node_t *node;

    size_t actual;
    node = heap_alloc(sizeof(heapmem_node_t) + size, &actual);

    node->magic     = HEAPMAGIC_AVAIL;
    node->len       = actual - sizeof(heapmem_node_t);
    node->allocated = false;
    node->next      = NULL;

    return node;
}

void heap_split_node(heapmem_node_t *node, size_t where) {
    if (!node)
        return;

    if (!where || node->len <= where)
        return;

    size_t rem = node->len - where;

    heapmem_node_t *new = heapnode_init(rem);

    new->next = node->next;

    node->len  -= rem;
    node->next  = new;
}

bool heap_check_header(heapmem_node_t *node) {
    int dead = (node->magic == HEAPMAGIC_UNAV);
    int good = (node->magic == HEAPMAGIC_AVAIL);

    return (dead == 0) || (good == 0);
}

// TODO: print initialization & other debug

void *PREFIX(malloc)(size_t size) {
#ifdef HEAP_DEBUG
    heap_debug("malloc(%#zx)\n", size);
#endif

    heap_lock();
    if (!root_node) {
#ifdef HEAP_DEBUG
        heap_debug("Initializing heap version %d.%d\n", HEAPVER_MAJOR,
                   HEAPVER_MINOR);
#endif
        root_node = heapnode_init(HEAP_PAGES * page_size);
#ifdef HEAP_DEBUG
        heap_debug("Root node initialized OK\n");
#endif
    }

    heapmem_node_t *cur;
    for (cur = root_node; cur != NULL; cur = cur->next) {
        if (!heap_check_header(cur)) {
#ifdef HEAP_DEBUG
            heap_debug("Header %#lx is invalid\n", cur->magic);
#endif
            return NULL;
        }

        bool is_magic_available = (cur->magic == HEAPMAGIC_AVAIL);
        bool can_size_fit       = (cur->len >= size);

        if (is_magic_available && can_size_fit)
            break;

#ifdef HEAP_DEBUG
        heap_debug("%p(%#zx) is not %#lx\n", cur, cur->len, HEAPMAGIC_AVAIL);
#endif

        // here: either it's already allocated or it doesn't fit
        if (!cur->next) {
            // we do this IF there's no ->next
            heapmem_node_t *new = heapnode_init(size);
            cur->next           = new;
#ifdef HEAP_DEBUG
            heap_debug("New node %p created with length %#zx\n", cur->next,
                       cur->next->len);
#endif
        }
    }

    if (!cur) // (for some unknown reason)
        return NULL;

    cur->magic = HEAPMAGIC_UNAV;

    if (cur->len > size) {
        heap_split_node(cur, size);

#ifdef HEAP_DEBUG
        heap_debug("Node %p has been split: %p(%#zx)\n", cur, cur->next,
                   cur->next->len);
#endif
    }

    cur->allocated = true;

    heap_unlock();

    void *p = HEAP_ALIGN((void *)cur);

    memset(p, 0, size);

#ifdef HEAP_DEBUG
    heap_debug("Return %p\n", p);
#endif

    return p;
}

void PREFIX(free)(void *ptr) {
#ifdef HEAP_DEBUG
    heap_debug("free(%p)\n", ptr);
#endif

    heapmem_node_t *deallocated = HEAP_UNALIGN(ptr);

    if (!heap_check_header(deallocated)) {
        // error here too
        return;
    }

    bool is_magic_allocated = (deallocated->magic == HEAPMAGIC_UNAV);

    if (!is_magic_allocated && !deallocated->allocated) {
        // error
        return;
    }
    heap_lock();

    deallocated->magic     = HEAPMAGIC_AVAIL;
    deallocated->allocated = false;

    // TODO: coalesce

    heap_unlock();

    // we're done :D
}

void *PREFIX(calloc)(size_t times, size_t size) {
#ifdef HEAP_DEBUG
    heap_debug("calloc(%#zx * %#zx)\n", times, size);
#endif
    return PREFIX(malloc)(times * size);
}

void *PREFIX(realloc)(void *p_old, size_t size) {
#ifdef HEAP_DEBUG
    heap_debug("realloc(%p, %#zx)\n", p_old, size);
#endif

    void *p_new = PREFIX(malloc)(size);

    void *p_old_realigned = HEAP_UNALIGN(p_old);
    heapmem_node_t *cur;
    for (cur = root_node; cur != NULL || p_old_realigned != cur;
         cur = cur->next)
        ;

    size_t copy_size = MIN(size, cur->len);

    memcpy(p_new, p_old, copy_size);

    PREFIX(free)(p_old);

    return NULL;
}
